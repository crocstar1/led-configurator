#include "hardware_config.h"

namespace {

static constexpr uint16_t FACTORY_ROWS = 8;
static constexpr uint16_t FACTORY_COLS = 12;

static_assert(DEFAULT_ACTIVE_PORT_COUNT >= 1,
              "The factory active port count must be non-zero");
static_assert(DEFAULT_ACTIVE_PORT_COUNT <= MAX_PORT_COUNT,
              "The factory active port count exceeds MAX_PORT_COUNT");
static_assert(static_cast<size_t>(FACTORY_ROWS) * FACTORY_COLS <=
                  MAX_LED_COUNT,
              "The factory matrix exceeds MAX_LED_COUNT");
static_assert(static_cast<size_t>(FACTORY_ROWS) * FACTORY_COLS >=
                  DEFAULT_ACTIVE_PORT_COUNT,
              "The factory matrix has fewer pixels than active ports");
static_assert(static_cast<uint8_t>(LedProtocol::WS2811_800K) == 1,
              "The persisted WS2811 protocol ID must remain stable");
static_assert(static_cast<uint8_t>(LedColorOrder::RGB) == 0 &&
                  static_cast<uint8_t>(LedColorOrder::RBG) == 1 &&
                  static_cast<uint8_t>(LedColorOrder::GRB) == 2 &&
                  static_cast<uint8_t>(LedColorOrder::GBR) == 3 &&
                  static_cast<uint8_t>(LedColorOrder::BRG) == 4 &&
                  static_cast<uint8_t>(LedColorOrder::BGR) == 5,
              "The persisted color-order IDs must remain stable");

}  // namespace

HardwareConfig hardware_config_factory_defaults() {
  return HardwareConfig{
      FACTORY_ROWS,
      FACTORY_COLS,
      DEFAULT_ACTIVE_PORT_COUNT,
      LedProtocol::WS2811_800K,
      LedColorOrder::BRG,
  };
}

bool hardware_config_calculate_led_count(const HardwareConfig &config,
                                         size_t &activeLedCount) {
  if (config.rows == 0 || config.cols == 0) {
    return false;
  }

  const size_t rows = config.rows;
  const size_t cols = config.cols;
  if (rows > MAX_LED_COUNT / cols) {
    return false;
  }

  const size_t calculatedLedCount = rows * cols;
  if (calculatedLedCount > MAX_LED_COUNT ||
      calculatedLedCount < config.activePortCount) {
    return false;
  }

  activeLedCount = calculatedLedCount;
  return true;
}

bool hardware_config_led_output_supported(LedProtocol protocol,
                                          LedColorOrder colorOrder) {
  if (protocol != LedProtocol::WS2811_800K) {
    return false;
  }

  switch (colorOrder) {
    case LedColorOrder::RGB:
    case LedColorOrder::RBG:
    case LedColorOrder::GRB:
    case LedColorOrder::GBR:
    case LedColorOrder::BRG:
    case LedColorOrder::BGR:
      return true;
    default:
      return false;
  }
}

bool hardware_config_validate(const HardwareConfig &config) {
  if (config.activePortCount < 1 ||
      config.activePortCount > MAX_PORT_COUNT) {
    return false;
  }

  size_t activeLedCount = 0;
  return hardware_config_calculate_led_count(config, activeLedCount) &&
         hardware_config_led_output_supported(config.ledProtocol,
                                              config.colorOrder);
}
