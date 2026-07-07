#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include "usp1_board_config.h"
#include "zone_model.h"

// Физические параметры матрицы 
#define LED_PIN         USP1_PRIMARY_LED_PIN
#define MATRIX_X        12      
#define VIRTUAL_Y       8       
#define NUM_IC_CHIPS    (MATRIX_X * VIRTUAL_Y) 

// Электрические лимиты для защиты DC-DC платы УСП-1
#define MAX_LED_VOLTS   12      
#define MAX_LED_MILLIAMPS 3000  

// Stored matrix rendering configuration.
struct MatrixConfig {
    uint8_t topology;        // 0 - Справа-Снизу, 1 - Слева-Снизу, 2 - Слева-Сверху, 3 - Справа-Сверху
    uint8_t logo_anim;       // 0 - Статичный цвет, 1 - Бегущая радуга FastLED
    
    uint8_t bright_ports;    // Яркость зон зарядок (1-255)
    uint8_t bright_logo;     // Яркость зоны логотипа (1-255)
    
    // Global RGB colors for port statuses.
    uint8_t color_wait[3];   // RGB для статуса "Ожидание"
    uint8_t color_charge[3]; // RGB для статуса "Зарядка"
    uint8_t color_error[3];  // RGB для статуса "Авария"
    uint8_t color_logo[3];   // RGB для статичного цвета Логотипа
    
    // Legacy matrix config fields kept until MatrixConfigV2 migration.
    uint8_t is_dhcp;         // 1 - DHCP, 0 - Static IP
    uint8_t static_ip[4];    // Массив байт IP
    uint8_t static_mask[4];  // Массив байт Маски
    uint8_t static_gw[4];    // Массив байт Шлюза
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
void led_clear_all_safe();
void led_load_config_from_flash(); 
void led_refresh_safe();
void led_set_pixel_zone_safe(int x, int y, uint8_t zoneId);
void led_reload_status_layers_safe();
void led_reload_free_zone_layers_safe();
int getLEDIndex(int x, int virtualY);

#endif
