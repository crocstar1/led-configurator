#include "matrix_config_storage.h"
#include <Preferences.h>
#include <ctype.h>
#include <string.h>
#include "zone_model.h"

static constexpr const char *NVS_NAMESPACE = "led_settings";
static constexpr const char *KEY_MATRIX_CONFIG = "matrix_cfg";
static constexpr const char *KEY_FREE_ZONES = "free_zones_v1";
static constexpr const char *KEY_LAYER_WAIT = "layer_wait";
static constexpr const char *KEY_LAYER_CHARGE = "layer_chg";
static constexpr const char *KEY_LAYER_ERROR = "layer_err";

static constexpr uint32_t MATRIX_CONFIG_MAGIC = 0x314D444CUL; // "LDM1"
static constexpr uint16_t MATRIX_CONFIG_VERSION = 1;
static constexpr uint32_t FREE_ZONE_CONFIG_MAGIC = 0x315A5246UL; // "FRZ1"
static constexpr uint16_t FREE_ZONE_CONFIG_VERSION = 1;
static constexpr uint8_t MAX_ZONE_ID = ZONE_ID_MAX;
static constexpr size_t MAX_LAYER_JSON_BYTES = 2 + (NUM_IC_CHIPS * 32);

static bool zones_are_valid(const uint8_t *zones, size_t zoneCount);

struct StoredMatrixConfig {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint16_t matrixX;
    uint16_t matrixY;
    MatrixConfig config;
    uint8_t zones[NUM_IC_CHIPS];
};

struct StoredFreeZoneConfigV1 {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint16_t matrixX;
    uint16_t matrixY;
    FreeZoneConfig zones[FREE_ZONE_COUNT];
};

static bool is_valid_hex_color(const String &value) {
    if (value.length() != 7 || value[0] != '#') {
        return false;
    }

    for (int i = 1; i < 7; i++) {
        if (!isxdigit((unsigned char)value[i])) {
            return false;
        }
    }
    return true;
}

static void skip_json_whitespace(const String &value, int &position) {
    while (position < value.length() && isspace((unsigned char)value[position])) {
        position++;
    }
}

static bool parse_coordinate_component(const String &value, int &result) {
    if (value.length() == 0) return false;

    result = 0;
    for (int i = 0; i < value.length(); i++) {
        if (!isdigit((unsigned char)value[i])) return false;
        const int digit = value[i] - '0';
        if (result > (2147483647 - digit) / 10) return false;
        result = (result * 10) + digit;
    }
    return true;
}

static bool parse_layer_coordinate(const String &coordinate, int &x, int &y) {
    const int dash = coordinate.indexOf('-');
    if (dash <= 0 || dash != coordinate.lastIndexOf('-') || dash >= coordinate.length() - 1) {
        return false;
    }

    return parse_coordinate_component(coordinate.substring(0, dash), x) &&
        parse_coordinate_component(coordinate.substring(dash + 1), y) &&
        x >= 0 && x < MATRIX_X && y >= 0 && y < VIRTUAL_Y;
}

static bool normalize_layer_json(
    const String &colorsJson,
    String &normalizedJson,
    bool enforceZone,
    uint8_t zoneId,
    const uint8_t *zones,
    size_t zoneCount
) {
    String source = colorsJson;
    source.trim();
    if (source.length() < 2 || source.length() > MAX_LAYER_JSON_BYTES ||
        source[0] != '{' || source[source.length() - 1] != '}') {
        return false;
    }

    if (enforceZone && (!zone_is_free(zoneId) || !zones_are_valid(zones, zoneCount))) {
        return false;
    }

    bool seen[NUM_IC_CHIPS] = {};
    size_t entryCount = 0;
    int position = 1;
    normalizedJson = "{";
    normalizedJson.reserve(source.length());

    skip_json_whitespace(source, position);
    if (position == source.length() - 1) {
        normalizedJson += "}";
        return true;
    }

    while (position < source.length() - 1) {
        if (source[position] != '"') return false;
        const int keyEnd = source.indexOf('"', position + 1);
        if (keyEnd < 0) return false;
        const String coordinate = source.substring(position + 1, keyEnd);
        position = keyEnd + 1;

        skip_json_whitespace(source, position);
        if (position >= source.length() || source[position] != ':') return false;
        position++;
        skip_json_whitespace(source, position);

        if (position >= source.length() || source[position] != '"') return false;
        const int valueEnd = source.indexOf('"', position + 1);
        if (valueEnd < 0) return false;
        const String color = source.substring(position + 1, valueEnd);
        position = valueEnd + 1;

        int x = -1;
        int y = -1;
        if (!parse_layer_coordinate(coordinate, x, y) || !is_valid_hex_color(color)) {
            return false;
        }

        const int index = getLEDIndex(x, y);
        if (index < 0 || index >= NUM_IC_CHIPS || seen[index] ||
            (enforceZone && zones[index] != zoneId)) {
            return false;
        }
        seen[index] = true;

        if (entryCount >= NUM_IC_CHIPS) return false;
        if (entryCount > 0) normalizedJson += ",";
        normalizedJson += "\"";
        normalizedJson += String(x);
        normalizedJson += "-";
        normalizedJson += String(y);
        normalizedJson += "\":\"";
        normalizedJson += color;
        normalizedJson += "\"";
        entryCount++;

        skip_json_whitespace(source, position);
        if (position == source.length() - 1) {
            normalizedJson += "}";
            return true;
        }
        if (source[position] != ',') return false;
        position++;
        skip_json_whitespace(source, position);
        if (position >= source.length() - 1) return false;
    }

    return false;
}

static const char *status_layer_key(const char *status) {
    if (status == nullptr) {
        return nullptr;
    }

    if (strcmp(status, "waiting") == 0 || strcmp(status, "wait") == 0) {
        return KEY_LAYER_WAIT;
    }

    if (strcmp(status, "charging") == 0 || strcmp(status, "charge") == 0) {
        return KEY_LAYER_CHARGE;
    }

    if (strcmp(status, "error") == 0 || strcmp(status, "err") == 0) {
        return KEY_LAYER_ERROR;
    }

    return nullptr;
}

static String free_zone_layer_key(uint8_t zoneId) {
    if (!zone_is_free(zoneId)) {
        return "";
    }

    String key = "fz";
    key += String((int)zoneId);
    key += "_layer";
    return key;
}

static void remove_free_zone_layers(Preferences &preferences) {
    for (size_t i = 0; i < FREE_ZONE_COUNT; i++) {
        const String key = free_zone_layer_key(FREE_ZONE_IDS[i]);
        if (key.length() > 0) preferences.remove(key.c_str());
    }
}

static void clear_incompatible_matrix_storage() {
    Preferences preferences;
    preferences.begin(NVS_NAMESPACE, false);
    preferences.remove(KEY_MATRIX_CONFIG);
    preferences.remove(KEY_FREE_ZONES);
    preferences.remove(KEY_LAYER_WAIT);
    preferences.remove(KEY_LAYER_CHARGE);
    preferences.remove(KEY_LAYER_ERROR);
    remove_free_zone_layers(preferences);
    preferences.end();
}

static void clear_incompatible_free_zone_storage() {
    Preferences preferences;
    preferences.begin(NVS_NAMESPACE, false);
    preferences.remove(KEY_FREE_ZONES);
    remove_free_zone_layers(preferences);
    preferences.end();
}

static bool zones_are_valid(const uint8_t *zones, size_t zoneCount) {
    if (zones == nullptr || zoneCount != NUM_IC_CHIPS) {
        return false;
    }

    for (size_t i = 0; i < zoneCount; i++) {
        if (zones[i] > MAX_ZONE_ID) {
            return false;
        }
    }
    return true;
}

static bool free_zone_config_is_valid(const FreeZoneConfig &zone) {
    if (!zone_is_free(zone.zoneId) || zone.enabled > 1 || zone.brightness == 0) {
        return false;
    }

    if (zone.mode != FREE_ZONE_STATIC &&
        zone.mode != FREE_ZONE_CUSTOM &&
        zone.mode != FREE_ZONE_RAINBOW) {
        return false;
    }

    return true;
}

static bool free_zone_configs_are_valid(const FreeZoneConfig *freeZones, size_t freeZoneCount) {
    if (freeZones == nullptr || freeZoneCount != FREE_ZONE_COUNT) {
        return false;
    }

    for (size_t i = 0; i < FREE_ZONE_COUNT; i++) {
        if (freeZones[i].zoneId != FREE_ZONE_IDS[i] || !free_zone_config_is_valid(freeZones[i])) {
            return false;
        }
    }

    return true;
}

static void matrix_config_set_defaults(MatrixConfig &config, uint8_t *zones, size_t zoneCount) {
    memset(&config, 0, sizeof(config));

    config.topology = 0;
    config.bright_ports = 120;

    if (zones != nullptr && zoneCount > 0) {
        const uint8_t activePorts = USP1_DEFAULT_ACTIVE_PORT_COUNT == 0
            ? 1
            : (USP1_DEFAULT_ACTIVE_PORT_COUNT > USP1_MAX_PORT_COUNT
                ? USP1_MAX_PORT_COUNT
                : USP1_DEFAULT_ACTIVE_PORT_COUNT);

        for (size_t i = 0; i < zoneCount; i++) {
            const uint8_t portOffset = (uint8_t)((i * activePorts) / zoneCount);
            zones[i] = (uint8_t)(ZONE_ID_PORT1 + portOffset);
        }
    }

    config.color_wait[0] = 0;
    config.color_wait[1] = 255;
    config.color_wait[2] = 0;

    config.color_charge[0] = 0;
    config.color_charge[1] = 80;
    config.color_charge[2] = 255;

    config.color_error[0] = 255;
    config.color_error[1] = 0;
    config.color_error[2] = 0;
}

static void matrix_config_set_free_zone_defaults(FreeZoneConfig *freeZones, size_t freeZoneCount) {
    if (freeZones == nullptr || freeZoneCount != FREE_ZONE_COUNT) {
        return;
    }

    for (size_t i = 0; i < FREE_ZONE_COUNT; i++) {
        const uint8_t zoneId = FREE_ZONE_IDS[i];
        const ZoneMetadata *metadata = zone_metadata_for(zoneId);
        freeZones[i].zoneId = zoneId;
        freeZones[i].enabled = 1;
        freeZones[i].mode = metadata ? metadata->defaultMode : FREE_ZONE_STATIC;
        freeZones[i].brightness = 100;
        freeZones[i].staticColor[0] = 255;
        freeZones[i].staticColor[1] = 255;
        freeZones[i].staticColor[2] = 255;
    }

    freeZones[free_zone_index(ZONE_ID_QR)].staticColor[0] = 52;
    freeZones[free_zone_index(ZONE_ID_QR)].staticColor[1] = 199;
    freeZones[free_zone_index(ZONE_ID_QR)].staticColor[2] = 89;

    freeZones[free_zone_index(ZONE_ID_SERVICE)].staticColor[0] = 255;
    freeZones[free_zone_index(ZONE_ID_SERVICE)].staticColor[1] = 149;
    freeZones[free_zone_index(ZONE_ID_SERVICE)].staticColor[2] = 0;

    freeZones[free_zone_index(ZONE_ID_CUSTOM)].staticColor[0] = 175;
    freeZones[free_zone_index(ZONE_ID_CUSTOM)].staticColor[1] = 82;
    freeZones[free_zone_index(ZONE_ID_CUSTOM)].staticColor[2] = 222;
}

static bool matrix_config_validate(const MatrixConfig &config, const uint8_t *zones, size_t zoneCount) {
    if (!zones_are_valid(zones, zoneCount)) {
        return false;
    }

    if (config.topology > 3) {
        return false;
    }

    if (config.bright_ports == 0) {
        return false;
    }

    return true;
}

static bool load_current_config(Preferences &preferences, MatrixConfig &config, uint8_t *zones, size_t zoneCount) {
    StoredMatrixConfig stored = {};
    const size_t bytesRead = preferences.getBytes(KEY_MATRIX_CONFIG, &stored, sizeof(stored));

    if (bytesRead != sizeof(stored) ||
        stored.magic != MATRIX_CONFIG_MAGIC ||
        stored.version != MATRIX_CONFIG_VERSION ||
        stored.size != sizeof(stored) ||
        stored.matrixX != MATRIX_X ||
        stored.matrixY != VIRTUAL_Y) {
        return false;
    }

    if (!matrix_config_validate(stored.config, stored.zones, sizeof(stored.zones))) {
        return false;
    }

    config = stored.config;
    memcpy(zones, stored.zones, zoneCount);
    return true;
}

bool matrix_config_load(MatrixConfig &config, uint8_t *zones, size_t zoneCount) {
    if (zones == nullptr || zoneCount != NUM_IC_CHIPS) {
        matrix_config_set_defaults(config, zones, zoneCount);
        return false;
    }

    Preferences preferences;
    preferences.begin(NVS_NAMESPACE, true);
    const bool loaded = load_current_config(preferences, config, zones, zoneCount);
    preferences.end();

    if (!loaded) {
        clear_incompatible_matrix_storage();
        matrix_config_set_defaults(config, zones, zoneCount);
    }

    return loaded;
}

bool matrix_config_save(const MatrixConfig &config, const uint8_t *zones, size_t zoneCount) {
    if (!matrix_config_validate(config, zones, zoneCount)) {
        return false;
    }

    StoredMatrixConfig stored = {};
    stored.magic = MATRIX_CONFIG_MAGIC;
    stored.version = MATRIX_CONFIG_VERSION;
    stored.size = sizeof(stored);
    stored.matrixX = MATRIX_X;
    stored.matrixY = VIRTUAL_Y;
    stored.config = config;
    memcpy(stored.zones, zones, sizeof(stored.zones));

    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    const size_t bytesWritten = preferences.putBytes(KEY_MATRIX_CONFIG, &stored, sizeof(stored));
    StoredMatrixConfig verified = {};
    const size_t bytesRead = preferences.getBytes(KEY_MATRIX_CONFIG, &verified, sizeof(verified));
    preferences.end();

    return bytesWritten == sizeof(stored) && bytesRead == sizeof(verified) &&
        memcmp(&stored, &verified, sizeof(stored)) == 0;
}

bool matrix_config_load_free_zones(FreeZoneConfig *freeZones, size_t freeZoneCount) {
    if (freeZones == nullptr || freeZoneCount != FREE_ZONE_COUNT) {
        return false;
    }

    StoredFreeZoneConfigV1 stored = {};

    Preferences preferences;
    preferences.begin(NVS_NAMESPACE, true);
    const size_t bytesRead = preferences.getBytes(KEY_FREE_ZONES, &stored, sizeof(stored));
    preferences.end();

    if (bytesRead != sizeof(stored) ||
        stored.magic != FREE_ZONE_CONFIG_MAGIC ||
        stored.version != FREE_ZONE_CONFIG_VERSION ||
        stored.size != sizeof(stored) ||
        stored.matrixX != MATRIX_X ||
        stored.matrixY != VIRTUAL_Y ||
        !free_zone_configs_are_valid(stored.zones, FREE_ZONE_COUNT)) {
        clear_incompatible_free_zone_storage();
        matrix_config_set_free_zone_defaults(freeZones, freeZoneCount);
        return false;
    }

    memcpy(freeZones, stored.zones, sizeof(stored.zones));
    return true;
}

bool matrix_config_save_free_zones(const FreeZoneConfig *freeZones, size_t freeZoneCount) {
    if (!free_zone_configs_are_valid(freeZones, freeZoneCount)) {
        return false;
    }

    StoredFreeZoneConfigV1 stored = {};
    stored.magic = FREE_ZONE_CONFIG_MAGIC;
    stored.version = FREE_ZONE_CONFIG_VERSION;
    stored.size = sizeof(stored);
    stored.matrixX = MATRIX_X;
    stored.matrixY = VIRTUAL_Y;
    memcpy(stored.zones, freeZones, sizeof(stored.zones));

    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    const size_t bytesWritten = preferences.putBytes(KEY_FREE_ZONES, &stored, sizeof(stored));
    StoredFreeZoneConfigV1 verified = {};
    const size_t bytesRead = preferences.getBytes(KEY_FREE_ZONES, &verified, sizeof(verified));
    preferences.end();

    return bytesWritten == sizeof(stored) && bytesRead == sizeof(verified) &&
        memcmp(&stored, &verified, sizeof(stored)) == 0;
}

String matrix_config_load_status_layer(const char *status) {
    const char *key = status_layer_key(status);
    if (key == nullptr) {
        return "{}";
    }

    Preferences preferences;
    preferences.begin(NVS_NAMESPACE, true);
    String layer = preferences.getString(key, "{}");
    preferences.end();

    String normalized;
    if (!matrix_config_normalize_status_layer(layer, normalized)) {
        return "{}";
    }

    return normalized;
}

bool matrix_config_save_status_layer(const char *status, const String &colorsJson) {
    const char *key = status_layer_key(status);
    if (key == nullptr) {
        return false;
    }

    String safeJson;
    if (!matrix_config_normalize_status_layer(colorsJson, safeJson)) {
        return false;
    }

    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    const size_t bytesWritten = preferences.putString(key, safeJson);
    const String verified = preferences.getString(key, "");
    preferences.end();

    return bytesWritten == safeJson.length() && verified == safeJson;
}

bool matrix_config_normalize_status_layer(const String &colorsJson, String &normalizedJson) {
    return normalize_layer_json(colorsJson, normalizedJson, false, ZONE_ID_OFF, nullptr, 0);
}

String matrix_config_load_free_zone_layer(uint8_t zoneId) {
    const String key = free_zone_layer_key(zoneId);
    if (key.length() == 0) {
        return "{}";
    }

    Preferences preferences;
    preferences.begin(NVS_NAMESPACE, true);
    String layer = preferences.getString(key.c_str(), "{}");
    preferences.end();

    String normalized;
    if (!normalize_layer_json(layer, normalized, false, zoneId, nullptr, 0)) {
        return "{}";
    }

    return normalized;
}

bool matrix_config_save_free_zone_layer(uint8_t zoneId, const String &colorsJson, const uint8_t *zones, size_t zoneCount) {
    const String key = free_zone_layer_key(zoneId);
    if (key.length() == 0 || !zones_are_valid(zones, zoneCount)) {
        return false;
    }

    String safeJson;
    if (!matrix_config_normalize_free_zone_layer(zoneId, colorsJson, zones, zoneCount, safeJson)) {
        return false;
    }

    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    const size_t bytesWritten = preferences.putString(key.c_str(), safeJson);
    const String verified = preferences.getString(key.c_str(), "");
    preferences.end();

    return bytesWritten == safeJson.length() && verified == safeJson;
}

bool matrix_config_normalize_free_zone_layer(
    uint8_t zoneId,
    const String &colorsJson,
    const uint8_t *zones,
    size_t zoneCount,
    String &normalizedJson
) {
    return normalize_layer_json(colorsJson, normalizedJson, true, zoneId, zones, zoneCount);
}
