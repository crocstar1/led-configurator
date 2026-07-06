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

#endif
