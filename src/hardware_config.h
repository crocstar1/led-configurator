#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <stddef.h>
#include <stdint.h>

#include "board_config.h"

static constexpr size_t MAX_LED_COUNT_PER_OUTPUT = 500;
static constexpr size_t MAX_TOTAL_LED_COUNT = 1000;

enum class LedProtocol : uint8_t {
  WS2811_800K = 1,
};

enum class LedColorOrder : uint8_t {
  RGB = 0,
  RBG = 1,
  GRB = 2,
  GBR = 3,
  BRG = 4,
  BGR = 5,
};

struct HardwareConfig {
  uint16_t rows;
  uint16_t cols;
  uint8_t activePortCount;
  LedProtocol ledProtocol;
  LedColorOrder colorOrder;
};

HardwareConfig hardware_config_factory_defaults();

bool hardware_config_calculate_led_count(const HardwareConfig &config,
                                         size_t &activeLedCount);

bool hardware_config_led_output_supported(LedProtocol protocol,
                                          LedColorOrder colorOrder);

bool hardware_config_validate(const HardwareConfig &config);

#endif
