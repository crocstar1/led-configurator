#include "matrix_config_storage.h"
#include <Preferences.h>
#include <ctype.h>
#include <string.h>
#include "zone_model.h"

static constexpr const char *NVS_NAMESPACE = "led_settings";
static constexpr const char *KEY_VERSIONED_CONFIG = "matrix_v1";
static constexpr const char *KEY_FREE_ZONES = "free_zones_v1";
static constexpr const char *KEY_LEGACY_ZONE_MAP = "zone_map";
static constexpr const char *KEY_LEGACY_CONFIG = "zone_cfg";
static constexpr const char *KEY_LAYER_WAIT = "layer_wait";
static constexpr const char *KEY_LAYER_CHARGE = "layer_chg";
static constexpr const char *KEY_LAYER_ERROR = "layer_err";

static constexpr uint32_t MATRIX_CONFIG_MAGIC = 0x31505355UL; // "USP1"
static constexpr uint16_t MATRIX_CONFIG_VERSION = 1;
static constexpr uint32_t FREE_ZONE_CONFIG_MAGIC = 0x315A5246UL; // "FRZ1"
static constexpr uint16_t FREE_ZONE_CONFIG_VERSION = 1;
static constexpr uint8_t MAX_ZONE_ID = ZONE_ID_MAX;

struct StoredMatrixConfigV1 {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
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

static void parse_hex_color_bytes(const String &hex, uint8_t *target) {
    if (target == nullptr || !is_valid_hex_color(hex)) {
        return;
    }

    for (int channel = 0; channel < 3; channel++) {
        char buf[3] = {hex[1 + channel * 2], hex[2 + channel * 2], '\0'};
        target[channel] = (uint8_t)strtoul(buf, nullptr, 16);
    }
}

static String rgb_to_hex_string(const uint8_t *rgb) {
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", rgb[0], rgb[1], rgb[2]);
    return String(buf);
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

void matrix_config_set_defaults(MatrixConfig &config, uint8_t *zones, size_t zoneCount) {
    if (zones != nullptr) {
        memset(zones, 1, zoneCount);
    }

    memset(&config, 0, sizeof(config));

    config.topology = 0;
    config.logo_anim = 1;
    config.bright_ports = 120;
    config.bright_logo = 100;

    config.color_wait[0] = 0;
    config.color_wait[1] = 255;
    config.color_wait[2] = 0;

    config.color_charge[0] = 0;
    config.color_charge[1] = 80;
    config.color_charge[2] = 255;

    config.color_error[0] = 255;
    config.color_error[1] = 0;
    config.color_error[2] = 0;

    config.color_logo[0] = 255;
    config.color_logo[1] = 255;
    config.color_logo[2] = 255;

    config.is_dhcp = 1;
    config.static_ip[0] = 192;
    config.static_ip[1] = 168;
    config.static_ip[2] = 1;
    config.static_ip[3] = 150;
    config.static_mask[0] = 255;
    config.static_mask[1] = 255;
    config.static_mask[2] = 255;
    config.static_mask[3] = 0;
    config.static_gw[0] = 192;
    config.static_gw[1] = 168;
    config.static_gw[2] = 1;
    config.static_gw[3] = 1;
}

void matrix_config_set_free_zone_defaults(const MatrixConfig &config, FreeZoneConfig *freeZones, size_t freeZoneCount) {
    if (freeZones == nullptr || freeZoneCount != FREE_ZONE_COUNT) {
        return;
    }

    for (size_t i = 0; i < FREE_ZONE_COUNT; i++) {
        const uint8_t zoneId = FREE_ZONE_IDS[i];
        const ZoneMetadata *metadata = zone_metadata_for(zoneId);
        freeZones[i].zoneId = zoneId;
        freeZones[i].enabled = 1;
        freeZones[i].mode = metadata ? metadata->defaultMode : FREE_ZONE_STATIC;
        freeZones[i].brightness = config.bright_logo > 0 ? config.bright_logo : 100;
        freeZones[i].staticColor[0] = 255;
        freeZones[i].staticColor[1] = 255;
        freeZones[i].staticColor[2] = 255;
    }

    freeZones[free_zone_index(ZONE_ID_LOGO)].mode = (config.logo_anim == 1) ? FREE_ZONE_RAINBOW : FREE_ZONE_STATIC;
    memcpy(freeZones[free_zone_index(ZONE_ID_LOGO)].staticColor, config.color_logo, 3);

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

bool matrix_config_validate(const MatrixConfig &config, const uint8_t *zones, size_t zoneCount) {
    if (!zones_are_valid(zones, zoneCount)) {
        return false;
    }

    if (config.topology > 3) {
        return false;
    }

    if (config.logo_anim > 1) {
        return false;
    }

    if (config.bright_ports == 0 || config.bright_logo == 0) {
        return false;
    }

    if (config.is_dhcp > 1) {
        return false;
    }

    return true;
}

static bool load_versioned_config(Preferences &preferences, MatrixConfig &config, uint8_t *zones, size_t zoneCount) {
    StoredMatrixConfigV1 stored = {};
    const size_t bytesRead = preferences.getBytes(KEY_VERSIONED_CONFIG, &stored, sizeof(stored));

    if (bytesRead != sizeof(stored) ||
        stored.magic != MATRIX_CONFIG_MAGIC ||
        stored.version != MATRIX_CONFIG_VERSION ||
        stored.size != sizeof(stored)) {
        return false;
    }

    if (!matrix_config_validate(stored.config, stored.zones, sizeof(stored.zones))) {
        return false;
    }

    config = stored.config;
    memcpy(zones, stored.zones, zoneCount);
    return true;
}

static bool load_legacy_config(Preferences &preferences, MatrixConfig &config, uint8_t *zones, size_t zoneCount) {
    MatrixConfig legacyConfig = {};
    uint8_t legacyZones[NUM_IC_CHIPS] = {};

    const size_t mapLen = preferences.getBytes(KEY_LEGACY_ZONE_MAP, legacyZones, sizeof(legacyZones));
    const size_t cfgLen = preferences.getBytes(KEY_LEGACY_CONFIG, &legacyConfig, sizeof(legacyConfig));

    if (mapLen != sizeof(legacyZones) || cfgLen != sizeof(legacyConfig)) {
        return false;
    }

    if (!matrix_config_validate(legacyConfig, legacyZones, sizeof(legacyZones))) {
        return false;
    }

    config = legacyConfig;
    memcpy(zones, legacyZones, zoneCount);
    return true;
}

bool matrix_config_load(MatrixConfig &config, uint8_t *zones, size_t zoneCount) {
    if (zones == nullptr || zoneCount != NUM_IC_CHIPS) {
        matrix_config_set_defaults(config, zones, zoneCount);
        return false;
    }

    Preferences preferences;
    preferences.begin(NVS_NAMESPACE, true);
    const bool loaded = load_versioned_config(preferences, config, zones, zoneCount) ||
                        load_legacy_config(preferences, config, zones, zoneCount);
    preferences.end();

    if (!loaded) {
        matrix_config_set_defaults(config, zones, zoneCount);
    }

    return loaded;
}

bool matrix_config_save(const MatrixConfig &config, const uint8_t *zones, size_t zoneCount) {
    if (!matrix_config_validate(config, zones, zoneCount)) {
        return false;
    }

    StoredMatrixConfigV1 stored = {};
    stored.magic = MATRIX_CONFIG_MAGIC;
    stored.version = MATRIX_CONFIG_VERSION;
    stored.size = sizeof(stored);
    stored.config = config;
    memcpy(stored.zones, zones, sizeof(stored.zones));

    Preferences preferences;
    preferences.begin(NVS_NAMESPACE, false);
    const size_t bytesWritten = preferences.putBytes(KEY_VERSIONED_CONFIG, &stored, sizeof(stored));
    preferences.end();

    return bytesWritten == sizeof(stored);
}

bool matrix_config_load_free_zones(const MatrixConfig &config, FreeZoneConfig *freeZones, size_t freeZoneCount) {
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
        matrix_config_set_free_zone_defaults(config, freeZones, freeZoneCount);
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
    preferences.begin(NVS_NAMESPACE, false);
    const size_t bytesWritten = preferences.putBytes(KEY_FREE_ZONES, &stored, sizeof(stored));
    preferences.end();

    return bytesWritten == sizeof(stored);
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

    if (layer.length() == 0) {
        return "{}";
    }

    return layer;
}

bool matrix_config_save_status_layer(const char *status, const String &colorsJson) {
    const char *key = status_layer_key(status);
    if (key == nullptr) {
        return false;
    }

    String safeJson = colorsJson;
    safeJson.trim();
    if (!safeJson.startsWith("{") || !safeJson.endsWith("}")) {
        return false;
    }

    Preferences preferences;
    preferences.begin(NVS_NAMESPACE, false);
    const size_t bytesWritten = preferences.putString(key, safeJson);
    preferences.end();

    return bytesWritten > 0;
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

    if (layer.length() == 0 || !layer.startsWith("{") || !layer.endsWith("}")) {
        return "{}";
    }

    return layer;
}

bool matrix_config_save_free_zone_layer(uint8_t zoneId, const String &colorsJson, const uint8_t *zones, size_t zoneCount) {
    const String key = free_zone_layer_key(zoneId);
    if (key.length() == 0 || !zones_are_valid(zones, zoneCount)) {
        return false;
    }

    String source = colorsJson;
    source.trim();
    if (!source.startsWith("{") || !source.endsWith("}")) {
        return false;
    }

    String safeJson = "{";
    bool first = true;
    int pos = 0;

    while (true) {
        int keyStart = source.indexOf('"', pos);
        if (keyStart < 0) break;

        int keyEnd = source.indexOf('"', keyStart + 1);
        if (keyEnd < 0) break;

        String coord = source.substring(keyStart + 1, keyEnd);
        int dashPos = coord.indexOf('-');
        if (dashPos < 0) {
            pos = keyEnd + 1;
            continue;
        }

        int colonPos = source.indexOf(':', keyEnd + 1);
        if (colonPos < 0) break;

        int valueStart = source.indexOf('"', colonPos + 1);
        if (valueStart < 0) break;

        int valueEnd = source.indexOf('"', valueStart + 1);
        if (valueEnd < 0) break;

        String value = source.substring(valueStart + 1, valueEnd);
        const int x = coord.substring(0, dashPos).toInt();
        const int y = coord.substring(dashPos + 1).toInt();
        const int index = getLEDIndex(x, y);

        if (index >= 0 && index < (int)zoneCount && zones[index] == zoneId && is_valid_hex_color(value)) {
            if (!first) safeJson += ",";
            first = false;
            safeJson += "\"";
            safeJson += String(x);
            safeJson += "-";
            safeJson += String(y);
            safeJson += "\":\"";
            safeJson += value;
            safeJson += "\"";
        }

        pos = valueEnd + 1;
    }

    safeJson += "}";

    Preferences preferences;
    preferences.begin(NVS_NAMESPACE, false);
    const size_t bytesWritten = preferences.putString(key.c_str(), safeJson);
    preferences.end();

    return bytesWritten > 0 || safeJson == "{}";
}
