#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <Arduino.h>

static constexpr uint8_t BOARD_INPUT_COUNT = 8;
static constexpr uint8_t LED_OUTPUT_COUNT = 4;
static constexpr uint8_t MAX_PORT_COUNT = 4;
static constexpr uint8_t DEFAULT_ACTIVE_PORT_COUNT = 2;

// USP-1 BOOT service button: external 10 kOhm pull-up, pressed to GND.
static constexpr uint8_t BOARD_SERVICE_BUTTON_PIN = 0;
static constexpr uint8_t BOARD_SERVICE_BUTTON_ACTIVE_LEVEL = LOW;

// Board optocoupler outputs are active-low at the ESP32 GPIO.
static constexpr bool BOARD_INPUT_ACTIVE_LOW = true;

static constexpr uint8_t BOARD_INPUT_PINS[BOARD_INPUT_COUNT] = {
    16, // Data1
    17, // Data2
    18, // Data3
    19, // Data4
    21, // Data5
    25, // Data6
    26, // Data7
    27  // Data8
};

static constexpr uint8_t LED_OUTPUT_PINS[LED_OUTPUT_COUNT] = {
    22, // LED1
    23, // LED2
    32, // LED3
    33  // LED4
};

#endif
