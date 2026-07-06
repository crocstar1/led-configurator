#include "matrix_config_storage.h"
#include <Preferences.h>
#include <string.h>
#include "zone_model.h"

static constexpr const char *NVS_NAMESPACE = "led_settings";
static constexpr const char *KEY_VERSIONED_CONFIG = "matrix_v1";
static constexpr const char *KEY_LEGACY_ZONE_MAP = "zone_map";
static constexpr const char *KEY_LEGACY_CONFIG = "zone_cfg";
static constexpr const char *KEY_LAYER_WAIT = "layer_wait";
static constexpr const char *KEY_LAYER_CHARGE = "layer_chg";
static constexpr const char *KEY_LAYER_ERROR = "layer_err";

static constexpr uint32_t MATRIX_CONFIG_MAGIC = 0x31505355UL; // "USP1"
static constexpr uint16_t MATRIX_CONFIG_VERSION = 1;
static constexpr uint8_t MAX_ZONE_ID = ZONE_ID_MAX;

struct StoredMatrixConfigV1 {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    MatrixConfig config;
    uint8_t zones[NUM_IC_CHIPS];
};

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
