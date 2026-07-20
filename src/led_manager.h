#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include "board_config.h"
#include "zone_model.h"

// Физические параметры матрицы 
#define LED_PIN         PRIMARY_LED_PIN
#define MATRIX_X        12      
#define VIRTUAL_Y       8       
#define NUM_IC_CHIPS    (MATRIX_X * VIRTUAL_Y) 

// LED matrix power budget used by FastLED brightness scaling.
#define MAX_LED_VOLTS   12      
#define MAX_LED_MILLIAMPS 3000  

// Stored matrix rendering configuration.
struct MatrixConfig {
    uint8_t topology;        // 0 - Справа-Снизу, 1 - Слева-Снизу, 2 - Слева-Сверху, 3 - Справа-Сверху
    uint8_t bright_ports;    // Общая яркость портовых зон (1-255)
    
    // Global RGB colors for port statuses.
    uint8_t color_wait[3];   // RGB для статуса "Ожидание"
    uint8_t color_charge[3]; // RGB для статуса "Зарядка"
    uint8_t color_error[3];  // RGB для статуса "Авария"
};

struct FreeZoneConfig {
    uint8_t zoneId;
    uint8_t enabled;
    FreeZoneMode mode;
    uint8_t staticColor[3];
    uint8_t brightness;
};

// Делаем переменные доступными для main.cpp
extern MatrixConfig cfg;
extern FreeZoneConfig freeZoneConfigs[FREE_ZONE_COUNT];
extern uint8_t zoneMap[NUM_IC_CHIPS];

// Инициализация и запуск фонового таска FreeRTOS на Core 1
void led_setup();

// Thread-safe LED and zone update helpers.
void led_refresh_safe();
void led_replace_zone_map_safe(const uint8_t *zones, size_t zoneCount);
void led_apply_matrix_config_safe(const MatrixConfig &config, const uint8_t *zones, size_t zoneCount);
void led_reload_status_layers_safe();
void led_reload_free_zone_layers_safe();
bool led_remap_zone_map_for_topology(
    const uint8_t *sourceZones,
    size_t sourceCount,
    uint8_t oldTopology,
    uint8_t newTopology,
    uint8_t *targetZones,
    size_t targetCount
);
int getLEDIndexForTopology(int x, int virtualY, uint8_t topology);
int getLEDIndex(int x, int virtualY);

#endif
