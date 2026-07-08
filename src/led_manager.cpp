#include "led_manager.h"
#include <FastLED.h>
#include <ctype.h>
#include "matrix_config_storage.h"
#include "status_mapper.h"
#include "zone_model.h"

CRGB leds[NUM_IC_CHIPS]; 
uint8_t zoneMap[NUM_IC_CHIPS]; 
MatrixConfig cfg;
FreeZoneConfig freeZoneConfigs[FREE_ZONE_COUNT];

float breatheScale = 0.2; 
bool breatheDirectionUp = true;
unsigned long lastBreatheTime = 0;

SemaphoreHandle_t ledMutex = NULL;

struct StatusLayerCache {
    bool hasColor[NUM_IC_CHIPS];
    CRGB colors[NUM_IC_CHIPS];
};

StatusLayerCache waitLayer;
StatusLayerCache chargeLayer;
StatusLayerCache errorLayer;

struct FreeZoneLayerCache {
    bool hasColor[NUM_IC_CHIPS];
    CRGB colors[NUM_IC_CHIPS];
};

FreeZoneLayerCache freeZoneLayers[FREE_ZONE_COUNT];

// Map logical matrix coordinates to the physical LED chain.
int getLEDIndex(int x, int virtualY) {
    if (x < 0 || x >= MATRIX_X || virtualY < 0 || virtualY >= VIRTUAL_Y) return -1;
    
    int tx = x;
    int ty = virtualY;

    switch (cfg.topology) {
        case 0: // Right-bottom start, vertical snake.
            tx = (MATRIX_X - 1) - x;
            if (tx % 2 == 0) return (tx * VIRTUAL_Y) + ty;
            else return (tx * VIRTUAL_Y) + ((VIRTUAL_Y - 1) - ty);
            
        case 1: // Left-bottom start, vertical snake.
            if (tx % 2 == 0) return (tx * VIRTUAL_Y) + ty;
            else return (tx * VIRTUAL_Y) + ((VIRTUAL_Y - 1) - ty);
            
        case 2: // Left-top start, horizontal snake.
            ty = (VIRTUAL_Y - 1) - virtualY;
            if (ty % 2 == 0) return (ty * MATRIX_X) + tx;
            else return (ty * MATRIX_X) + ((MATRIX_X - 1) - tx);
            
        case 3: // Right-top start, horizontal snake.
            tx = (MATRIX_X - 1) - x;
            ty = (VIRTUAL_Y - 1) - virtualY;
            if (ty % 2 == 0) return (ty * MATRIX_X) + tx;
            else return (ty * MATRIX_X) + ((MATRIX_X - 1) - tx);
            
        default:
            return -1;
    }
}

int hexDigitValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

bool parseHexColorToCrgb(const String &hex, CRGB &color) {
    if (hex.length() != 7 || hex[0] != '#') {
        return false;
    }

    uint8_t rgb[3] = {0, 0, 0};
    for (int channel = 0; channel < 3; channel++) {
        int hi = hexDigitValue(hex[1 + channel * 2]);
        int lo = hexDigitValue(hex[2 + channel * 2]);
        if (hi < 0 || lo < 0) {
            return false;
        }
        rgb[channel] = (uint8_t)((hi << 4) | lo);
    }

    color = CRGB(rgb[0], rgb[1], rgb[2]);
    return true;
}

void clearStatusLayer(StatusLayerCache &layer) {
    memset(layer.hasColor, 0, sizeof(layer.hasColor));
    for (int i = 0; i < NUM_IC_CHIPS; i++) {
        layer.colors[i] = CRGB::Black;
    }
}

void clearFreeZoneLayer(FreeZoneLayerCache &layer) {
    memset(layer.hasColor, 0, sizeof(layer.hasColor));
    for (int i = 0; i < NUM_IC_CHIPS; i++) {
        layer.colors[i] = CRGB::Black;
    }
}

void parseStatusLayerJson(const String &json, StatusLayerCache &layer) {
    clearStatusLayer(layer);

    int pos = 0;
    while (true) {
        int keyStart = json.indexOf('"', pos);
        if (keyStart < 0) break;

        int keyEnd = json.indexOf('"', keyStart + 1);
        if (keyEnd < 0) break;

        String key = json.substring(keyStart + 1, keyEnd);
        int dashPos = key.indexOf('-');
        if (dashPos < 0) {
            pos = keyEnd + 1;
            continue;
        }

        int colonPos = json.indexOf(':', keyEnd + 1);
        if (colonPos < 0) break;

        int valueStart = json.indexOf('"', colonPos + 1);
        if (valueStart < 0) break;

        int valueEnd = json.indexOf('"', valueStart + 1);
        if (valueEnd < 0) break;

        String value = json.substring(valueStart + 1, valueEnd);
        CRGB color;
        int x = key.substring(0, dashPos).toInt();
        int y = key.substring(dashPos + 1).toInt();
        int index = getLEDIndex(x, y);

        if (index >= 0 && parseHexColorToCrgb(value, color)) {
            layer.hasColor[index] = true;
            layer.colors[index] = color;
        }

        pos = valueEnd + 1;
    }
}

void parseFreeZoneLayerJson(uint8_t zoneId, const String &json, FreeZoneLayerCache &layer) {
    clearFreeZoneLayer(layer);

    int pos = 0;
    while (true) {
        int keyStart = json.indexOf('"', pos);
        if (keyStart < 0) break;

        int keyEnd = json.indexOf('"', keyStart + 1);
        if (keyEnd < 0) break;

        String key = json.substring(keyStart + 1, keyEnd);
        int dashPos = key.indexOf('-');
        if (dashPos < 0) {
            pos = keyEnd + 1;
            continue;
        }

        int colonPos = json.indexOf(':', keyEnd + 1);
        if (colonPos < 0) break;

        int valueStart = json.indexOf('"', colonPos + 1);
        if (valueStart < 0) break;

        int valueEnd = json.indexOf('"', valueStart + 1);
        if (valueEnd < 0) break;

        String value = json.substring(valueStart + 1, valueEnd);
        CRGB color;
        int x = key.substring(0, dashPos).toInt();
        int y = key.substring(dashPos + 1).toInt();
        int index = getLEDIndex(x, y);

        if (index >= 0 && zoneMap[index] == zoneId && parseHexColorToCrgb(value, color)) {
            layer.hasColor[index] = true;
            layer.colors[index] = color;
        }

        pos = valueEnd + 1;
    }
}

void led_reload_status_layers_unlocked() {
    parseStatusLayerJson(matrix_config_load_status_layer("waiting"), waitLayer);
    parseStatusLayerJson(matrix_config_load_status_layer("charging"), chargeLayer);
    parseStatusLayerJson(matrix_config_load_status_layer("error"), errorLayer);
}

void led_reload_free_zone_layers_unlocked() {
    for (size_t i = 0; i < FREE_ZONE_COUNT; i++) {
        parseFreeZoneLayerJson(freeZoneConfigs[i].zoneId, matrix_config_load_free_zone_layer(freeZoneConfigs[i].zoneId), freeZoneLayers[i]);
    }
}

bool getStatusLayerColorForStatus(int index, PortStatus status, CRGB &color) {
    StatusLayerCache *layer = nullptr;

    if (status == PORT_STATUS_WAITING) {
        layer = &waitLayer;
    } else if (status == PORT_STATUS_CHARGING) {
        layer = &chargeLayer;
    } else if (status == PORT_STATUS_ERROR) {
        layer = &errorLayer;
    }

    if (layer == nullptr || index < 0 || index >= NUM_IC_CHIPS || !layer->hasColor[index]) {
        return false;
    }

    color = layer->colors[index];
    return true;
}

FreeZoneConfig *freeZoneConfigFor(uint8_t zoneId) {
    const int index = free_zone_index(zoneId);
    if (index < 0) {
        return nullptr;
    }
    return &freeZoneConfigs[index];
}

bool getFreeZoneLayerColor(uint8_t zoneId, int index, CRGB &color) {
    const int zoneIndex = free_zone_index(zoneId);
    if (zoneIndex < 0 || index < 0 || index >= NUM_IC_CHIPS || !freeZoneLayers[zoneIndex].hasColor[index]) {
        return false;
    }

    color = freeZoneLayers[zoneIndex].colors[index];
    return true;
}

CRGB visibleErrorColor(CRGB preferred) {
    if ((uint16_t)preferred.r + preferred.g + preferred.b < 40) {
        return CRGB(255, 0, 0);
    }
    return preferred;
}

void led_refresh_internal() {
    static uint8_t rainbowHue = 0;
    rainbowHue += 1; 

    // Вычисляем синусоидальный пульс яркости для режима ожидания
    uint8_t pulseBright = (uint8_t)(220 * breatheScale);
    if (pulseBright < 40) pulseBright = 40; 

    // Извлекаем кастомные цвета портов из конфига и масштабируем их под яркость bright_ports
    CRGB customWait   = CRGB(cfg.color_wait[0], cfg.color_wait[1], cfg.color_wait[2]);
    CRGB customCharge = CRGB(cfg.color_charge[0], cfg.color_charge[1], cfg.color_charge[2]);
    CRGB customError  = CRGB(cfg.color_error[0], cfg.color_error[1], cfg.color_error[2]);

    // Применяем индивидуальные лимиты яркости зон (ШИМ-масштабирование FastLED)
    customWait.nscale8_video(cfg.bright_ports);
    customCharge.nscale8_video(cfg.bright_ports);
    customError.nscale8_video(cfg.bright_ports);

    const StatusMapperState &portState = status_mapper_get_state();

    for (int i = 0; i < NUM_IC_CHIPS; i++) {
        uint8_t zone = zoneMap[i]; 

        if (zone == 0) {
            leds[i] = CRGB::Black; 
        } 
        else {
            CRGB layerColor;
            if (zone_is_port(zone)) {
                const uint8_t portIndex = zone - ZONE_ID_PORT1;
                const PortStatus zoneStatus = (portIndex < USP1_MAX_PORT_COUNT)
                    ? portState.statuses[portIndex]
                    : PORT_STATUS_DISABLED;

                if (zoneStatus == PORT_STATUS_DISABLED) {
                    leds[i] = CRGB::Black;
                    continue;
                }

                if (getStatusLayerColorForStatus(i, zoneStatus, layerColor)) {
                    layerColor.nscale8_video(cfg.bright_ports);

                    if (zoneStatus == PORT_STATUS_ERROR) {
                        layerColor = visibleErrorColor(layerColor);
                        leds[i] = ((millis() / 500) % 2 == 0) ? layerColor : CRGB::Black;
                    } else if (zoneStatus == PORT_STATUS_WAITING) {
                        layerColor.nscale8_video(pulseBright);
                        leds[i] = layerColor;
                    } else {
                        leds[i] = layerColor;
                    }
                    continue;
                }
            }
        }
        // СЕРВИСНЫЕ ЗОНЫ ЗАРЯДНЫХ ПОРТОВ (1, 2, 3, 4)
        if (zone_is_port(zone)) {
            const uint8_t portIndex = zone - ZONE_ID_PORT1;
            const PortStatus zoneStatus = (portIndex < USP1_MAX_PORT_COUNT)
                ? portState.statuses[portIndex]
                : PORT_STATUS_DISABLED;

            if (zoneStatus == PORT_STATUS_DISABLED) {
                leds[i] = CRGB::Black;
            } else if (zoneStatus == PORT_STATUS_ERROR) {
                // Моргаем кастомным цветом ошибки (раз в 500 мс)
                leds[i] = ((millis() / 500) % 2 == 0) ? visibleErrorColor(customError) : CRGB::Black;
            } else if (zoneStatus == PORT_STATUS_CHARGING) {
                leds[i] = customCharge; // Горит кастомным цветом зарядки
            } else {
                CRGB dynamicWait = customWait;
                dynamicWait.nscale8_video(pulseBright);
                leds[i] = dynamicWait;
            }
        } 
        // Free zones: Logo, QR highlight, Service, Custom.
        else if (zone_is_free(zone)) {
            FreeZoneConfig *freeConfig = freeZoneConfigFor(zone);
            if (freeConfig == nullptr || freeConfig->enabled == 0) {
                leds[i] = CRGB::Black;
            } else if (freeConfig->mode == FREE_ZONE_RAINBOW) {
                leds[i] = CHSV(rainbowHue + (i * 4), 255, freeConfig->brightness);
            } else if (freeConfig->mode == FREE_ZONE_CUSTOM) {
                CRGB layerColor;
                if (getFreeZoneLayerColor(zone, i, layerColor)) {
                    layerColor.nscale8_video(freeConfig->brightness);
                    leds[i] = layerColor;
                } else {
                    leds[i] = CRGB::Black;
                }
            } else {
                CRGB staticColor = CRGB(freeConfig->staticColor[0], freeConfig->staticColor[1], freeConfig->staticColor[2]);
                staticColor.nscale8_video(freeConfig->brightness);
                leds[i] = staticColor;
            }
        }
    }
    FastLED.show();
}

void ledTaskWorker(void * parameter) {
    while (true) {
        unsigned long now = millis();
        if (xSemaphoreTake(ledMutex, (TickType_t)10) == pdTRUE) {

            if (now - lastBreatheTime > 25) {
                lastBreatheTime = now;
                if (breatheDirectionUp) {
                    breatheScale += 0.012;
                    if (breatheScale >= 1.0) breatheDirectionUp = false;
                } else {
                    breatheScale -= 0.012;
                    if (breatheScale <= 0.15) breatheDirectionUp = true;
                }
                led_refresh_internal();
            }

            xSemaphoreGive(ledMutex);
        }
        vTaskDelay(15 / portTICK_PERIOD_MS);
    }
}

void led_setup() {
    ledMutex = xSemaphoreCreateMutex();
    
    FastLED.addLeds<WS2811, LED_PIN, BRG>(leds, NUM_IC_CHIPS); 
    FastLED.setMaxPowerInVoltsAndMilliamps(MAX_LED_VOLTS, MAX_LED_MILLIAMPS);
    
    led_load_config_from_flash();
    led_refresh_internal();

    xTaskCreatePinnedToCore(ledTaskWorker, "LED_Task", 4096, NULL, 1, NULL, 1);
}

void led_load_config_from_flash() {
    matrix_config_load(cfg, zoneMap, sizeof(zoneMap));
    matrix_config_load_free_zones(freeZoneConfigs, FREE_ZONE_COUNT);
    led_reload_status_layers_unlocked();
    led_reload_free_zone_layers_unlocked();

    for (int i = 0; i < NUM_IC_CHIPS; i++) leds[i] = CRGB::Black;
    FastLED.show();
}

void led_refresh_safe() {
    if (ledMutex == NULL) return;
    if (xSemaphoreTake(ledMutex, portMAX_DELAY) == pdTRUE) {
        led_refresh_internal();
        xSemaphoreGive(ledMutex);
    }
}

void led_set_pixel_zone_safe(int x, int y, uint8_t zoneId) {
    if (!zone_id_is_valid(zoneId)) return;
    if (ledMutex == NULL) return;
    if (xSemaphoreTake(ledMutex, portMAX_DELAY) == pdTRUE) {
        int index = getLEDIndex(x, y);
        if (index != -1) {
            zoneMap[index] = zoneId;
        }
        xSemaphoreGive(ledMutex);
    }
}

void led_replace_zone_map_safe(const uint8_t *zones, size_t zoneCount) {
    if (zones == nullptr || zoneCount != NUM_IC_CHIPS) return;

    if (ledMutex == NULL) {
        memcpy(zoneMap, zones, sizeof(zoneMap));
        return;
    }

    if (xSemaphoreTake(ledMutex, portMAX_DELAY) == pdTRUE) {
        memcpy(zoneMap, zones, sizeof(zoneMap));
        led_refresh_internal();
        xSemaphoreGive(ledMutex);
    }
}

void led_reload_status_layers_safe() {
    if (ledMutex == NULL) {
        led_reload_status_layers_unlocked();
        return;
    }

    if (xSemaphoreTake(ledMutex, portMAX_DELAY) == pdTRUE) {
        led_reload_status_layers_unlocked();
        led_refresh_internal();
        xSemaphoreGive(ledMutex);
    }
}

void led_reload_free_zone_layers_safe() {
    if (ledMutex == NULL) {
        led_reload_free_zone_layers_unlocked();
        return;
    }

    if (xSemaphoreTake(ledMutex, portMAX_DELAY) == pdTRUE) {
        led_reload_free_zone_layers_unlocked();
        led_refresh_internal();
        xSemaphoreGive(ledMutex);
    }
}
