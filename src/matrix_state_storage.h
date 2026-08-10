#ifndef MATRIX_STATE_STORAGE_H
#define MATRIX_STATE_STORAGE_H

#include <stddef.h>
#include <stdint.h>
#include <type_traits>

#include "hardware_config.h"
#include "zone_model.h"

static constexpr uint32_t MATRIX_STATE_MAGIC = 0x32534D54UL;  // "TMS2"
static constexpr uint16_t MATRIX_STATE_FORMAT_VERSION = 2;
static constexpr size_t MATRIX_STATE_LAYER_WAITING = 0;
static constexpr size_t MATRIX_STATE_LAYER_CHARGING = 1;
static constexpr size_t MATRIX_STATE_LAYER_ERROR = 2;
static constexpr size_t MATRIX_STATE_LAYER_LOGO = 3;
static constexpr size_t MATRIX_STATE_LAYER_QR = 4;
static constexpr size_t MATRIX_STATE_LAYER_SERVICE = 5;
static constexpr size_t MATRIX_STATE_LAYER_CUSTOM = 6;
static constexpr size_t MATRIX_STATE_LAYER_COUNT = 7;
static constexpr size_t MATRIX_STATE_LAYER_MASK_BYTES =
    (MAX_LED_COUNT + 7U) / 8U;
static constexpr size_t MATRIX_STATE_LAYER_RGB_BYTES = MAX_LED_COUNT * 3U;

enum class MatrixStateValidationResult : uint8_t {
  VALID,
  INCOMPATIBLE,
  INVALID,
};

enum class MatrixStateLoadResult : uint8_t {
  LOADED,
  NOT_FOUND,
  INCOMPATIBLE,
  CORRUPT,
  STORAGE_ERROR,
};

enum class MatrixStateSaveResult : uint8_t {
  SAVED,
  INVALID_STATE,
  STORAGE_ERROR,
  VERIFY_FAILED,
};

struct MatrixStateStoredFreeZone {
  uint8_t zoneId;
  uint8_t enabled;
  uint8_t mode;
  uint8_t brightness;
  uint8_t staticColor[3];
  uint8_t reserved;
};

struct MatrixStateStoredLayer {
  // Pixel i uses bit (i % 8), least-significant bit first.
  uint8_t present[MATRIX_STATE_LAYER_MASK_BYTES];
  uint8_t rgb[MATRIX_STATE_LAYER_RGB_BYTES];
};

// Fixed ESP32 little-endian V2 storage image. zoneMap and every layer use
// row-major logical coordinates: logicalIndex = y * cols + x. Topology is not
// applied to these arrays; it is only metadata for a later logical-to-physical
// LED mapping. Layer order is waiting, charging, error, Logo, QR, Service,
// Custom. A stored free-zone pixel may be hidden by the current zoneMap; future
// rendering must apply it only when that logical pixel belongs to its free zone,
// so hidden layer data can never override a port or its error indication.
struct MatrixStateV2 {
  uint32_t magic;
  uint16_t formatVersion;
  uint16_t structureSize;
  uint16_t rows;
  uint16_t cols;
  uint8_t topology;
  uint8_t portBrightness;
  uint8_t waitingColor[3];
  uint8_t chargingColor[3];
  uint8_t errorColor[3];
  uint8_t activePortCount;
  uint8_t ledProtocol;
  uint8_t colorOrder;
  uint8_t reservedHeader[6];
  MatrixStateStoredFreeZone freeZones[FREE_ZONE_COUNT];
  uint8_t zoneMap[MAX_LED_COUNT];
  MatrixStateStoredLayer layers[MATRIX_STATE_LAYER_COUNT];
  uint8_t reservedTail[3];
};

static constexpr size_t MATRIX_STATE_STORAGE_BYTES = 11508;

static_assert(MAX_LED_COUNT == 500,
              "The V2 storage layout is defined for 500 LEDs");
static_assert(MAX_PORT_COUNT == 4,
              "The V2 storage layout is defined for four ports");
static_assert(FREE_ZONE_COUNT == 4,
              "The V2 storage layout is defined for four free zones");
static_assert(MATRIX_STATE_LAYER_CUSTOM + 1U == MATRIX_STATE_LAYER_COUNT,
              "The V2 storage layout is defined for seven layers");
static_assert(sizeof(uint8_t) == 1 && sizeof(uint16_t) == 2 &&
                  sizeof(uint32_t) == 4,
              "Unexpected fixed-width integer sizes");
static_assert(sizeof(LedProtocol) == 1 && sizeof(LedColorOrder) == 1,
              "Unexpected hardware enum sizes");
static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
              "The V2 storage layout requires a little-endian target");
static_assert(sizeof(MatrixStateStoredFreeZone) == 8,
              "Unexpected free-zone storage layout");
static_assert(sizeof(MatrixStateStoredLayer) == 1563,
              "Unexpected layer storage layout");
static_assert(offsetof(MatrixStateV2, activePortCount) == 23,
              "Unexpected V2 active-port layout");
static_assert(offsetof(MatrixStateV2, ledProtocol) == 24,
              "Unexpected V2 LED-protocol layout");
static_assert(offsetof(MatrixStateV2, colorOrder) == 25,
              "Unexpected V2 color-order layout");
static_assert(offsetof(MatrixStateV2, reservedHeader) == 26,
              "Unexpected V2 reserved-header layout");
static_assert(offsetof(MatrixStateV2, freeZones) == 32,
              "Unexpected V2 metadata layout");
static_assert(offsetof(MatrixStateV2, zoneMap) == 64,
              "Unexpected V2 free-zone layout");
static_assert(offsetof(MatrixStateV2, layers) == 564,
              "Unexpected V2 zone-map layout");
static_assert(offsetof(MatrixStateV2, reservedTail) == 11505,
              "Unexpected V2 layer layout");
static_assert(sizeof(MatrixStateV2) == MATRIX_STATE_STORAGE_BYTES,
              "Unexpected V2 snapshot size");
static_assert(MATRIX_STATE_STORAGE_BYTES <= UINT16_MAX,
              "The V2 snapshot size must fit structureSize");
static_assert(std::is_standard_layout<MatrixStateV2>::value,
              "The V2 snapshot must have standard layout");
static_assert(std::is_trivially_copyable<MatrixStateV2>::value,
              "The V2 snapshot must be trivially copyable");

void matrix_state_storage_set_factory_defaults(MatrixStateV2 &state);

MatrixStateValidationResult matrix_state_storage_validate(
    const MatrixStateV2 &state);

// The caller must serialize storage access and must not mutate state while a
// synchronous save is in progress. Load changes state only when it returns
// LOADED. For every other result state is unchanged; the caller decides
// explicitly whether to apply factory defaults.
MatrixStateLoadResult matrix_state_storage_load(MatrixStateV2 &state);
MatrixStateSaveResult matrix_state_storage_save(const MatrixStateV2 &state);

#endif
