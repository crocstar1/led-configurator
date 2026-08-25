#ifndef MATRIX_STATE_STORAGE_H
#define MATRIX_STATE_STORAGE_H

#include <stddef.h>
#include <stdint.h>
#include <type_traits>

#include "hardware_config.h"
#include "zone_model.h"

static constexpr size_t MATRIX_STATE_LAYER_WAITING = 0;
static constexpr size_t MATRIX_STATE_LAYER_CHARGING = 1;
static constexpr size_t MATRIX_STATE_LAYER_ERROR = 2;
static constexpr size_t MATRIX_STATE_LAYER_LOGO = 3;
static constexpr size_t MATRIX_STATE_LAYER_QR = 4;
static constexpr size_t MATRIX_STATE_LAYER_SERVICE = 5;
static constexpr size_t MATRIX_STATE_LAYER_CUSTOM = 6;
static constexpr size_t MATRIX_STATE_LAYER_COUNT = 7;

static constexpr uint32_t MATRIX_STATE_V3_MAGIC = 0x33534D54UL;  // "TMS3"
static constexpr uint16_t MATRIX_STATE_V3_FORMAT_VERSION = 3;
static constexpr size_t MATRIX_STATE_V3_LAYER_COUNT = 7;
static constexpr size_t MATRIX_STATE_V3_LAYER_MASK_BYTES =
    (MAX_TOTAL_LED_COUNT + 7U) / 8U;
static constexpr size_t MATRIX_STATE_V3_LAYER_RGB_BYTES =
    MAX_TOTAL_LED_COUNT * 3U;
static constexpr size_t MATRIX_STATE_V3_LAYER_STORAGE_BYTES =
    MATRIX_STATE_V3_LAYER_MASK_BYTES + MATRIX_STATE_V3_LAYER_RGB_BYTES;
static constexpr size_t MATRIX_STATE_V3_STORAGE_BYTES = 23068;

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

struct MatrixStateV3Output {
  uint16_t rows;
  uint16_t cols;
  uint8_t enabled;
  uint8_t topology;
  uint8_t ledProtocol;
  uint8_t colorOrder;
};

// Derived runtime metadata. Offsets and lengths are never persisted.
struct MatrixStateV3OutputLayout {
  uint16_t offset;
  uint16_t length;
};

struct MatrixStateV3StoredLayer {
  // Pixel i uses bit (i % 8), least-significant bit first.
  uint8_t present[MATRIX_STATE_V3_LAYER_MASK_BYTES];
  // Pixel i occupies rgb[i * 3], rgb[i * 3 + 1], rgb[i * 3 + 2].
  uint8_t rgb[MATRIX_STATE_V3_LAYER_RGB_BYTES];
};

// Fixed ESP32 little-endian V3 model. Output segments are concatenated in
// LED1..LED4 order. An enabled output's global logical index is
// outputOffset + y * cols + x; disabled outputs have zero length.
struct MatrixStateV3 {
  uint32_t magic;
  uint16_t formatVersion;
  uint16_t structureSize;
  uint8_t activePortCount;
  uint8_t portBrightness;
  uint8_t waitingColor[3];
  uint8_t chargingColor[3];
  uint8_t errorColor[3];
  uint8_t reservedHeader[13];
  MatrixStateV3Output outputs[LED_OUTPUT_COUNT];
  // Persisted as [outputIndex][freeZoneIndex] for LED1..LED4.
  MatrixStateStoredFreeZone freeZones[LED_OUTPUT_COUNT][FREE_ZONE_COUNT];
  uint8_t zoneMap[MAX_TOTAL_LED_COUNT];
  MatrixStateV3StoredLayer layers[MATRIX_STATE_V3_LAYER_COUNT];
  uint8_t reservedTail[1];
};

static_assert(MAX_PORT_COUNT == 4,
              "The matrix-state layout is defined for four ports");
static_assert(FREE_ZONE_COUNT == 4,
              "The matrix-state layout is defined for four free zones");
static_assert(MATRIX_STATE_LAYER_CUSTOM + 1U == MATRIX_STATE_LAYER_COUNT,
              "The matrix-state layout is defined for seven layers");
static_assert(sizeof(uint8_t) == 1 && sizeof(uint16_t) == 2 &&
                  sizeof(uint32_t) == 4,
              "Unexpected fixed-width integer sizes");
static_assert(sizeof(LedProtocol) == 1 && sizeof(LedColorOrder) == 1,
              "Unexpected hardware enum sizes");
static_assert(sizeof(MatrixStateStoredFreeZone) == 8,
              "Unexpected free-zone storage layout");

static_assert(LED_OUTPUT_COUNT == 4,
              "The V3 layout is defined for four LED outputs");
static_assert(MAX_LED_COUNT_PER_OUTPUT == 500,
              "The V3 per-output capacity must remain 500");
static_assert(MAX_TOTAL_LED_COUNT == 1000,
              "The V3 total capacity must remain 1000");
static_assert(MATRIX_STATE_V3_LAYER_COUNT == 7,
              "The V3 layout is defined for seven layers");
static_assert(MATRIX_STATE_V3_LAYER_COUNT == MATRIX_STATE_LAYER_COUNT,
              "The persisted state must use all seven layer roles");
static_assert(MATRIX_STATE_V3_LAYER_MASK_BYTES == 125,
              "Unexpected V3 presence-mask size");
static_assert(MATRIX_STATE_V3_LAYER_RGB_BYTES == 3000,
              "Unexpected V3 RGB storage size");
static_assert(MATRIX_STATE_V3_LAYER_STORAGE_BYTES == 3125,
              "Unexpected V3 layer storage size");
static_assert(MATRIX_STATE_V3_LAYER_COUNT *
                      MATRIX_STATE_V3_LAYER_STORAGE_BYTES ==
                  21875,
              "Unexpected V3 combined layer storage size");
static_assert(MAX_TOTAL_LED_COUNT <= UINT16_MAX,
              "V3 offsets and lengths must fit uint16_t");
static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
              "The V3 storage layout requires a little-endian target");

static_assert(sizeof(MatrixStateV3Output) == 8,
              "Unexpected V3 output-descriptor size");
static_assert(offsetof(MatrixStateV3Output, rows) == 0,
              "Unexpected V3 output rows offset");
static_assert(offsetof(MatrixStateV3Output, cols) == 2,
              "Unexpected V3 output cols offset");
static_assert(offsetof(MatrixStateV3Output, enabled) == 4,
              "Unexpected V3 output enabled offset");
static_assert(offsetof(MatrixStateV3Output, topology) == 5,
              "Unexpected V3 output topology offset");
static_assert(offsetof(MatrixStateV3Output, ledProtocol) == 6,
              "Unexpected V3 output protocol offset");
static_assert(offsetof(MatrixStateV3Output, colorOrder) == 7,
              "Unexpected V3 output color-order offset");
static_assert(std::is_standard_layout<MatrixStateV3Output>::value,
              "The V3 output descriptor must have standard layout");
static_assert(std::is_trivially_copyable<MatrixStateV3Output>::value,
              "The V3 output descriptor must be trivially copyable");

static_assert(sizeof(MatrixStateV3OutputLayout) == 4,
              "Unexpected derived V3 output-layout size");
static_assert(std::is_standard_layout<MatrixStateV3OutputLayout>::value,
              "The derived V3 output layout must have standard layout");
static_assert(std::is_trivially_copyable<MatrixStateV3OutputLayout>::value,
              "The derived V3 output layout must be trivially copyable");

static_assert(offsetof(MatrixStateV3StoredLayer, present) == 0,
              "Unexpected V3 presence-mask offset");
static_assert(offsetof(MatrixStateV3StoredLayer, rgb) == 125,
              "Unexpected V3 RGB offset");
static_assert(sizeof(MatrixStateV3StoredLayer) ==
                  MATRIX_STATE_V3_LAYER_STORAGE_BYTES,
              "Unexpected V3 layer size");
static_assert(std::is_standard_layout<MatrixStateV3StoredLayer>::value,
              "The V3 layer must have standard layout");
static_assert(std::is_trivially_copyable<MatrixStateV3StoredLayer>::value,
              "The V3 layer must be trivially copyable");

static_assert(std::extent<decltype(MatrixStateV3::outputs), 0>::value ==
                  LED_OUTPUT_COUNT,
              "V3 must contain exactly four output descriptors");
static_assert(sizeof(decltype(MatrixStateV3::outputs)) == 32,
              "Unexpected V3 combined output-descriptor size");
static_assert(std::extent<decltype(MatrixStateV3::freeZones), 0>::value ==
                      LED_OUTPUT_COUNT &&
                  std::extent<decltype(MatrixStateV3::freeZones), 1>::value ==
                      FREE_ZONE_COUNT,
              "V3 free zones must use [output][free-zone] order");
static_assert(sizeof(decltype(MatrixStateV3::freeZones)) == 128,
              "Unexpected V3 combined free-zone size");
static_assert(std::extent<decltype(MatrixStateV3::layers), 0>::value ==
                  MATRIX_STATE_V3_LAYER_COUNT,
              "V3 must contain exactly seven layers");

static_assert(offsetof(MatrixStateV3, magic) == 0,
              "Unexpected V3 magic offset");
static_assert(offsetof(MatrixStateV3, formatVersion) == 4,
              "Unexpected V3 format-version offset");
static_assert(offsetof(MatrixStateV3, structureSize) == 6,
              "Unexpected V3 structure-size offset");
static_assert(offsetof(MatrixStateV3, activePortCount) == 8,
              "Unexpected V3 active-port offset");
static_assert(offsetof(MatrixStateV3, portBrightness) == 9,
              "Unexpected V3 port-brightness offset");
static_assert(offsetof(MatrixStateV3, waitingColor) == 10,
              "Unexpected V3 waiting-color offset");
static_assert(offsetof(MatrixStateV3, chargingColor) == 13,
              "Unexpected V3 charging-color offset");
static_assert(offsetof(MatrixStateV3, errorColor) == 16,
              "Unexpected V3 error-color offset");
static_assert(offsetof(MatrixStateV3, reservedHeader) == 19,
              "Unexpected V3 reserved-header offset");
static_assert(offsetof(MatrixStateV3, outputs) == 32,
              "Unexpected V3 output-descriptor offset");
static_assert(offsetof(MatrixStateV3, freeZones) == 64,
              "Unexpected V3 free-zone offset");
static_assert(offsetof(MatrixStateV3, zoneMap) == 192,
              "Unexpected V3 zone-map offset");
static_assert(offsetof(MatrixStateV3, layers) == 1192,
              "Unexpected V3 layer offset");
static_assert(offsetof(MatrixStateV3, reservedTail) == 23067,
              "Unexpected V3 reserved-tail offset");
static_assert(sizeof(MatrixStateV3) == MATRIX_STATE_V3_STORAGE_BYTES,
              "Unexpected V3 snapshot size");
static_assert(MATRIX_STATE_V3_STORAGE_BYTES <= UINT16_MAX,
              "The V3 snapshot size must fit structureSize");
static_assert(std::is_standard_layout<MatrixStateV3>::value,
              "The V3 snapshot must have standard layout");
static_assert(std::is_trivially_copyable<MatrixStateV3>::value,
              "The V3 snapshot must be trivially copyable");

// Copies the immutable factory fallback for waiting, charging or error into
// color. Returns false for a non-status layer index.
bool matrix_state_storage_get_factory_status_color(
    size_t layerIndex,
    uint8_t color[3]);

bool matrix_state_storage_clear_saved_state();

bool matrix_state_v3_calculate_output_layout(
    const MatrixStateV3 &state,
    MatrixStateV3OutputLayout (&layout)[LED_OUTPUT_COUNT],
    size_t &totalLedCount);
void matrix_state_storage_set_v3_factory_defaults(MatrixStateV3 &state);
MatrixStateValidationResult matrix_state_storage_validate_v3(
    const MatrixStateV3 &state);
void matrix_state_storage_clear_v3_spatial_data(MatrixStateV3 &state);

// The caller must serialize storage access and must not mutate state while a
// synchronous save is in progress. Load changes state only when it returns
// LOADED. For every other result state is unchanged; the caller decides
// explicitly whether to apply factory defaults.
MatrixStateLoadResult matrix_state_storage_load_v3(MatrixStateV3 &state);
MatrixStateSaveResult matrix_state_storage_save_v3(
    const MatrixStateV3 &state);

#endif
