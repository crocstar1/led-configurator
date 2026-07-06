#ifndef ZONE_MODEL_H
#define ZONE_MODEL_H

#include <Arduino.h>
#include "usp1_board_config.h"

enum ZoneType : uint8_t {
    ZONE_TYPE_OFF = 0,
    ZONE_TYPE_PORT,
    ZONE_TYPE_FREE
};

enum FreeZoneMode : uint8_t {
    FREE_ZONE_STATIC = 0,
    FREE_ZONE_CUSTOM,
    FREE_ZONE_RAINBOW
};

static constexpr uint8_t ZONE_ID_OFF = 0;
static constexpr uint8_t ZONE_ID_PORT1 = 1;
static constexpr uint8_t ZONE_ID_PORT2 = 2;
static constexpr uint8_t ZONE_ID_PORT3 = 3;
static constexpr uint8_t ZONE_ID_PORT4 = 4;
static constexpr uint8_t ZONE_ID_LOGO = 5;
static constexpr uint8_t ZONE_ID_QR = 6;
static constexpr uint8_t ZONE_ID_SERVICE = 7;
static constexpr uint8_t ZONE_ID_CUSTOM = 8;
static constexpr uint8_t ZONE_ID_MAX = ZONE_ID_CUSTOM;

struct ZoneMetadata {
    uint8_t id;
    const char *name;
    ZoneType type;
    bool enabled;
    bool reserved;
    int8_t linkedPort;
    FreeZoneMode defaultMode;
};

static constexpr ZoneMetadata ZONE_METADATA[] = {
    {ZONE_ID_OFF, "Off", ZONE_TYPE_OFF, true, false, -1, FREE_ZONE_STATIC},
    {ZONE_ID_PORT1, "Port 1", ZONE_TYPE_PORT, true, false, 0, FREE_ZONE_STATIC},
    {ZONE_ID_PORT2, "Port 2", ZONE_TYPE_PORT, false, true, 1, FREE_ZONE_STATIC},
    {ZONE_ID_PORT3, "Port 3", ZONE_TYPE_PORT, false, true, 2, FREE_ZONE_STATIC},
    {ZONE_ID_PORT4, "Port 4", ZONE_TYPE_PORT, false, true, 3, FREE_ZONE_STATIC},
    {ZONE_ID_LOGO, "Logo", ZONE_TYPE_FREE, true, false, -1, FREE_ZONE_STATIC},
    {ZONE_ID_QR, "QR", ZONE_TYPE_FREE, true, false, -1, FREE_ZONE_STATIC},
    {ZONE_ID_SERVICE, "Service", ZONE_TYPE_FREE, true, false, -1, FREE_ZONE_STATIC},
    {ZONE_ID_CUSTOM, "Custom", ZONE_TYPE_FREE, true, false, -1, FREE_ZONE_CUSTOM},
};

static constexpr size_t ZONE_METADATA_COUNT = sizeof(ZONE_METADATA) / sizeof(ZONE_METADATA[0]);

inline bool zone_id_is_valid(uint8_t zoneId) {
    return zoneId <= ZONE_ID_MAX;
}

inline bool zone_is_port(uint8_t zoneId) {
    return zoneId >= ZONE_ID_PORT1 && zoneId <= ZONE_ID_PORT4;
}

inline bool zone_is_free(uint8_t zoneId) {
    return zoneId >= ZONE_ID_LOGO && zoneId <= ZONE_ID_CUSTOM;
}

inline const ZoneMetadata *zone_metadata_for(uint8_t zoneId) {
    for (size_t i = 0; i < ZONE_METADATA_COUNT; i++) {
        if (ZONE_METADATA[i].id == zoneId) {
            return &ZONE_METADATA[i];
        }
    }
    return nullptr;
}

inline const char *zone_type_to_string(ZoneType type) {
    switch (type) {
        case ZONE_TYPE_PORT:
            return "port";
        case ZONE_TYPE_FREE:
            return "free";
        case ZONE_TYPE_OFF:
        default:
            return "off";
    }
}

inline const char *free_zone_mode_to_string(FreeZoneMode mode) {
    switch (mode) {
        case FREE_ZONE_CUSTOM:
            return "custom";
        case FREE_ZONE_RAINBOW:
            return "rainbow";
        case FREE_ZONE_STATIC:
        default:
            return "static";
    }
}

#endif
