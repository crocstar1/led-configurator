#ifndef MATRIX_CONFIG_STORAGE_H
#define MATRIX_CONFIG_STORAGE_H

#include <Arduino.h>
#include "led_manager.h"

void matrix_config_set_defaults(MatrixConfig &config, uint8_t *zones, size_t zoneCount);
bool matrix_config_validate(const MatrixConfig &config, const uint8_t *zones, size_t zoneCount);
bool matrix_config_load(MatrixConfig &config, uint8_t *zones, size_t zoneCount);
bool matrix_config_save(const MatrixConfig &config, const uint8_t *zones, size_t zoneCount);
String matrix_config_load_status_layer(const char *status);
bool matrix_config_save_status_layer(const char *status, const String &colorsJson);
void matrix_config_set_free_zone_defaults(const MatrixConfig &config, FreeZoneConfig *freeZones, size_t freeZoneCount);
bool matrix_config_load_free_zones(const MatrixConfig &config, FreeZoneConfig *freeZones, size_t freeZoneCount);
bool matrix_config_save_free_zones(const FreeZoneConfig *freeZones, size_t freeZoneCount);
String matrix_config_load_free_zone_layer(uint8_t zoneId);
bool matrix_config_save_free_zone_layer(uint8_t zoneId, const String &colorsJson, const uint8_t *zones, size_t zoneCount);

#endif
