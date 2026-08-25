#include "matrix_state_storage.h"

#include <Arduino.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <string.h>

namespace {

static constexpr char MATRIX_NVS_PARTITION[] = "matrix_nvs";
static constexpr char MATRIX_NVS_NAMESPACE[] = "matrix";
static constexpr char MATRIX_NVS_KEY[] = "state_v2";

static constexpr uint8_t TOPOLOGY_MAX = 3;
static constexpr uint8_t DEFAULT_PORT_BRIGHTNESS = 120;
static constexpr uint8_t DEFAULT_FREE_ZONE_BRIGHTNESS = 100;
static constexpr uint8_t FACTORY_STATUS_COLORS[3][3] = {
    {0, 255, 0},
    {0, 80, 255},
    {255, 0, 0},
};

static_assert(MATRIX_STATE_LAYER_COUNT == 7,
              "The stored layer order must contain seven layers");
static_assert(MATRIX_STATE_LAYER_WAITING == 0 &&
                  MATRIX_STATE_LAYER_CHARGING == 1 &&
                  MATRIX_STATE_LAYER_ERROR == 2,
              "Factory status colors depend on the status-layer order");

constexpr bool v3EnabledOutputDimensionsAreValid(uint16_t rows,
                                                 uint16_t cols) {
  return rows != 0 && cols != 0 &&
         static_cast<size_t>(rows) <=
             MAX_LED_COUNT_PER_OUTPUT / static_cast<size_t>(cols);
}

constexpr bool v3CanAppendOutputLength(size_t currentTotal, size_t length) {
  return currentTotal <= MAX_TOTAL_LED_COUNT &&
         length <= MAX_LED_COUNT_PER_OUTPUT &&
         length <= MAX_TOTAL_LED_COUNT - currentTotal;
}

static_assert(v3EnabledOutputDimensionsAreValid(1, 500),
              "A 1x500 V3 output must be valid");
static_assert(v3EnabledOutputDimensionsAreValid(500, 1),
              "A 500x1 V3 output must be valid");
static_assert(!v3EnabledOutputDimensionsAreValid(501, 1),
              "A 501x1 V3 output must be rejected");
static_assert(!v3EnabledOutputDimensionsAreValid(UINT16_MAX, UINT16_MAX),
              "Oversized V3 dimensions must be rejected before multiplication");
static_assert(v3CanAppendOutputLength(0, 500) &&
                  v3CanAppendOutputLength(500, 500),
              "Two 500-pixel V3 outputs must fit the total limit");
static_assert(!v3CanAppendOutputLength(1000, 1),
              "A V3 total above 1000 pixels must be rejected");

// One reusable buffer keeps the 11.5 KiB image off task stacks. The module is
// deliberately synchronous and non-reentrant.
MatrixStateV2 storageBuffer;
bool matrixPartitionInitialized = false;

enum class ReadBlobResult : uint8_t {
  OK,
  NOT_FOUND,
  INCOMPATIBLE,
  CORRUPT,
  ERROR,
};

void logNvsError(const char *operation, esp_err_t error) {
  Serial.printf("matrix_nvs %s failed: %s (0x%04X)\n",
                operation,
                esp_err_to_name(error),
                static_cast<unsigned int>(error));
}

bool initializeMatrixPartition() {
  if (matrixPartitionInitialized) {
    return true;
  }

  bool erasedForRecovery = false;
  esp_err_t error = nvs_flash_init_partition(MATRIX_NVS_PARTITION);
  if (error == ESP_ERR_NVS_NO_FREE_PAGES ||
      error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    Serial.printf("matrix_nvs init returned %s; erasing only matrix_nvs\n",
                  esp_err_to_name(error));

    error = nvs_flash_erase_partition(MATRIX_NVS_PARTITION);
    if (error != ESP_OK) {
      logNvsError("erase", error);
      return false;
    }

    erasedForRecovery = true;
    error = nvs_flash_init_partition(MATRIX_NVS_PARTITION);
  }

  if (error != ESP_OK) {
    logNvsError("init", error);
    Serial.println(erasedForRecovery
                       ? "matrix_nvs was erased but could not be initialized"
                       : "matrix_nvs left unchanged");
    return false;
  }

  matrixPartitionInitialized = true;
  return true;
}

bool bytesAreZero(const uint8_t *bytes, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    if (bytes[i] != 0) {
      return false;
    }
  }
  return true;
}

bool calculateActiveLedCount(const MatrixStateV2 &state,
                             size_t &activeLedCount) {
  const HardwareConfig hardware = {
      state.rows,
      state.cols,
      state.activePortCount,
      static_cast<LedProtocol>(state.ledProtocol),
      static_cast<LedColorOrder>(state.colorOrder),
  };
  return hardware_config_validate(hardware) &&
         hardware_config_calculate_led_count(hardware, activeLedCount);
}

bool freeZoneIsValid(const MatrixStateStoredFreeZone &zone,
                     size_t index) {
  if (index >= FREE_ZONE_COUNT || zone.zoneId != FREE_ZONE_IDS[index] ||
      zone.enabled > 1 || zone.reserved != 0) {
    return false;
  }

  return zone.mode == FREE_ZONE_STATIC || zone.mode == FREE_ZONE_CUSTOM ||
         zone.mode == FREE_ZONE_RAINBOW;
}

bool layerHasPixel(const MatrixStateStoredLayer &layer, size_t pixel) {
  return (layer.present[pixel / 8U] & (1U << (pixel % 8U))) != 0;
}

bool layerIsValid(const MatrixStateStoredLayer &layer,
                  size_t activeLedCount) {
  for (size_t pixel = 0; pixel < MAX_LED_COUNT; ++pixel) {
    const bool present = layerHasPixel(layer, pixel);
    const size_t rgbOffset = pixel * 3U;
    const bool rgbIsZero = layer.rgb[rgbOffset] == 0 &&
                           layer.rgb[rgbOffset + 1U] == 0 &&
                           layer.rgb[rgbOffset + 2U] == 0;

    if (pixel >= activeLedCount) {
      if (present || !rgbIsZero) {
        return false;
      }
    } else if (!present && !rgbIsZero) {
      return false;
    }
  }

  for (size_t bit = MAX_LED_COUNT;
       bit < MATRIX_STATE_LAYER_MASK_BYTES * 8U;
       ++bit) {
    if ((layer.present[bit / 8U] & (1U << (bit % 8U))) != 0) {
      return false;
    }
  }

  return true;
}

ReadBlobResult readBlob(nvs_handle_t handle, MatrixStateV2 &state) {
  size_t storedSize = 0;
  esp_err_t error = nvs_get_blob(handle, MATRIX_NVS_KEY, nullptr, &storedSize);
  if (error == ESP_ERR_NVS_NOT_FOUND) {
    return ReadBlobResult::NOT_FOUND;
  }
  if (error == ESP_ERR_NVS_TYPE_MISMATCH) {
    return ReadBlobResult::INCOMPATIBLE;
  }
  if (error != ESP_OK) {
    logNvsError("read length", error);
    return ReadBlobResult::ERROR;
  }
  if (storedSize != sizeof(MatrixStateV2)) {
    return ReadBlobResult::INCOMPATIBLE;
  }

  memset(&state, 0, sizeof(state));
  size_t readSize = sizeof(state);
  error = nvs_get_blob(handle, MATRIX_NVS_KEY, &state, &readSize);
  if (error == ESP_ERR_NVS_NOT_FOUND) {
    return ReadBlobResult::CORRUPT;
  }
  if (error == ESP_ERR_NVS_TYPE_MISMATCH ||
      error == ESP_ERR_NVS_INVALID_LENGTH) {
    return ReadBlobResult::CORRUPT;
  }
  if (error != ESP_OK) {
    logNvsError("read", error);
    return ReadBlobResult::ERROR;
  }

  return readSize == sizeof(state) ? ReadBlobResult::OK
                                   : ReadBlobResult::INCOMPATIBLE;
}

void setFreeZoneDefaults(MatrixStateV2 &state) {
  static constexpr uint8_t DEFAULT_COLORS[FREE_ZONE_COUNT][3] = {
      {255, 255, 255},
      {52, 199, 89},
      {255, 149, 0},
      {175, 82, 222},
  };

  for (size_t i = 0; i < FREE_ZONE_COUNT; ++i) {
    MatrixStateStoredFreeZone &zone = state.freeZones[i];
    zone.zoneId = FREE_ZONE_IDS[i];
    zone.enabled = 1;
    const ZoneMetadata *metadata = zone_metadata_for(zone.zoneId);
    zone.mode = static_cast<uint8_t>(
        metadata == nullptr ? FREE_ZONE_STATIC : metadata->defaultMode);
    zone.brightness = DEFAULT_FREE_ZONE_BRIGHTNESS;
    memcpy(zone.staticColor, DEFAULT_COLORS[i], sizeof(zone.staticColor));
  }
}

void clearZoneMap(MatrixStateV2 &state) {
  memset(state.zoneMap, ZONE_ID_OFF, sizeof(state.zoneMap));
}

void setV3FreeZoneDefaults(
    MatrixStateStoredFreeZone (&zones)[FREE_ZONE_COUNT]) {
  static constexpr uint8_t DEFAULT_COLORS[FREE_ZONE_COUNT][3] = {
      {255, 255, 255},
      {52, 199, 89},
      {255, 149, 0},
      {175, 82, 222},
  };

  for (size_t i = 0; i < FREE_ZONE_COUNT; ++i) {
    MatrixStateStoredFreeZone &zone = zones[i];
    zone.zoneId = FREE_ZONE_IDS[i];
    zone.enabled = 1;
    const ZoneMetadata *metadata = zone_metadata_for(zone.zoneId);
    zone.mode = static_cast<uint8_t>(
        metadata == nullptr ? FREE_ZONE_STATIC : metadata->defaultMode);
    zone.brightness = DEFAULT_FREE_ZONE_BRIGHTNESS;
    memcpy(zone.staticColor, DEFAULT_COLORS[i], sizeof(zone.staticColor));
  }
}

bool v3LayerHasPixel(const MatrixStateV3StoredLayer &layer, size_t pixel) {
  return (layer.present[pixel / 8U] & (1U << (pixel % 8U))) != 0;
}

bool v3LayerIsValid(const MatrixStateV3StoredLayer &layer,
                    size_t totalLedCount) {
  for (size_t pixel = 0; pixel < MAX_TOTAL_LED_COUNT; ++pixel) {
    const bool present = v3LayerHasPixel(layer, pixel);
    const size_t rgbOffset = pixel * 3U;
    const bool rgbIsZero = layer.rgb[rgbOffset] == 0 &&
                           layer.rgb[rgbOffset + 1U] == 0 &&
                           layer.rgb[rgbOffset + 2U] == 0;

    if (pixel >= totalLedCount) {
      if (present || !rgbIsZero) {
        return false;
      }
    } else if (!present && !rgbIsZero) {
      return false;
    }
  }

  for (size_t bit = MAX_TOTAL_LED_COUNT;
       bit < MATRIX_STATE_V3_LAYER_MASK_BYTES * 8U;
       ++bit) {
    if ((layer.present[bit / 8U] & (1U << (bit % 8U))) != 0) {
      return false;
    }
  }

  return true;
}

}  // namespace

bool matrix_state_storage_get_factory_status_color(
    size_t layerIndex,
    uint8_t color[3]) {
  if (color == nullptr || layerIndex > MATRIX_STATE_LAYER_ERROR) {
    return false;
  }
  memcpy(color, FACTORY_STATUS_COLORS[layerIndex], 3U);
  return true;
}

void matrix_state_storage_set_factory_defaults(MatrixStateV2 &state) {
  memset(&state, 0, sizeof(state));

  const HardwareConfig hardware = hardware_config_factory_defaults();
  state.magic = MATRIX_STATE_MAGIC;
  state.formatVersion = MATRIX_STATE_FORMAT_VERSION;
  state.structureSize = static_cast<uint16_t>(sizeof(state));
  state.rows = hardware.rows;
  state.cols = hardware.cols;
  state.topology = 0;
  state.portBrightness = DEFAULT_PORT_BRIGHTNESS;
  state.activePortCount = hardware.activePortCount;
  state.ledProtocol = static_cast<uint8_t>(hardware.ledProtocol);
  state.colorOrder = static_cast<uint8_t>(hardware.colorOrder);

  memcpy(state.waitingColor,
         FACTORY_STATUS_COLORS[MATRIX_STATE_LAYER_WAITING],
         sizeof(state.waitingColor));
  memcpy(state.chargingColor,
         FACTORY_STATUS_COLORS[MATRIX_STATE_LAYER_CHARGING],
         sizeof(state.chargingColor));
  memcpy(state.errorColor,
         FACTORY_STATUS_COLORS[MATRIX_STATE_LAYER_ERROR],
         sizeof(state.errorColor));

  setFreeZoneDefaults(state);

  clearZoneMap(state);
}

bool matrix_state_storage_reset_spatial_data(
    MatrixStateV2 &state,
    uint16_t rows,
    uint16_t cols,
    uint8_t activePortCount) {
  if (matrix_state_storage_validate(state) !=
      MatrixStateValidationResult::VALID) {
    return false;
  }

  const HardwareConfig hardware = {
      rows,
      cols,
      activePortCount,
      static_cast<LedProtocol>(state.ledProtocol),
      static_cast<LedColorOrder>(state.colorOrder),
  };
  if (!hardware_config_validate(hardware)) {
    return false;
  }

  memcpy(&storageBuffer, &state, sizeof(storageBuffer));
  storageBuffer.rows = rows;
  storageBuffer.cols = cols;
  storageBuffer.activePortCount = activePortCount;
  memset(storageBuffer.layers, 0, sizeof(storageBuffer.layers));
  clearZoneMap(storageBuffer);

  if (matrix_state_storage_validate(storageBuffer) !=
      MatrixStateValidationResult::VALID) {
    return false;
  }

  memcpy(&state, &storageBuffer, sizeof(state));
  return true;
}

MatrixStateValidationResult matrix_state_storage_validate(
    const MatrixStateV2 &state) {
  if (state.magic != MATRIX_STATE_MAGIC ||
      state.formatVersion != MATRIX_STATE_FORMAT_VERSION ||
      state.structureSize != sizeof(MatrixStateV2)) {
    return MatrixStateValidationResult::INCOMPATIBLE;
  }

  size_t activeLedCount = 0;
  if (!calculateActiveLedCount(state, activeLedCount) ||
      state.topology > TOPOLOGY_MAX ||
      !bytesAreZero(state.reservedHeader, sizeof(state.reservedHeader)) ||
      !bytesAreZero(state.reservedTail, sizeof(state.reservedTail))) {
    return MatrixStateValidationResult::INVALID;
  }

  for (size_t i = 0; i < FREE_ZONE_COUNT; ++i) {
    if (!freeZoneIsValid(state.freeZones[i], i)) {
      return MatrixStateValidationResult::INVALID;
    }
  }

  for (size_t pixel = 0; pixel < MAX_LED_COUNT; ++pixel) {
    if (!zone_id_is_valid(state.zoneMap[pixel]) ||
        (pixel >= activeLedCount && state.zoneMap[pixel] != ZONE_ID_OFF)) {
      return MatrixStateValidationResult::INVALID;
    }
  }

  for (size_t pixel = 0; pixel < activeLedCount; ++pixel) {
    const uint8_t zoneId = state.zoneMap[pixel];
    if (!zone_is_port(zoneId)) {
      continue;
    }

    const uint8_t portIndex = zoneId - ZONE_ID_PORT1;
    if (portIndex >= state.activePortCount) {
      return MatrixStateValidationResult::INVALID;
    }
  }

  for (size_t i = 0; i < MATRIX_STATE_LAYER_COUNT; ++i) {
    if (!layerIsValid(state.layers[i], activeLedCount)) {
      return MatrixStateValidationResult::INVALID;
    }
  }

  return MatrixStateValidationResult::VALID;
}

MatrixStateLoadResult matrix_state_storage_load(MatrixStateV2 &state) {
  if (!initializeMatrixPartition()) {
    return MatrixStateLoadResult::STORAGE_ERROR;
  }

  nvs_handle_t handle = 0;
  const esp_err_t openError = nvs_open_from_partition(
      MATRIX_NVS_PARTITION, MATRIX_NVS_NAMESPACE, NVS_READONLY, &handle);
  if (openError == ESP_ERR_NVS_NOT_FOUND) {
    return MatrixStateLoadResult::NOT_FOUND;
  }
  if (openError != ESP_OK) {
    logNvsError("open for read", openError);
    return MatrixStateLoadResult::STORAGE_ERROR;
  }

  const ReadBlobResult readResult = readBlob(handle, storageBuffer);
  nvs_close(handle);

  if (readResult == ReadBlobResult::NOT_FOUND) {
    return MatrixStateLoadResult::NOT_FOUND;
  }
  if (readResult == ReadBlobResult::INCOMPATIBLE) {
    return MatrixStateLoadResult::INCOMPATIBLE;
  }
  if (readResult == ReadBlobResult::CORRUPT) {
    return MatrixStateLoadResult::CORRUPT;
  }
  if (readResult != ReadBlobResult::OK) {
    return MatrixStateLoadResult::STORAGE_ERROR;
  }

  const MatrixStateValidationResult validation =
      matrix_state_storage_validate(storageBuffer);
  if (validation == MatrixStateValidationResult::INCOMPATIBLE) {
    return MatrixStateLoadResult::INCOMPATIBLE;
  }
  if (validation != MatrixStateValidationResult::VALID) {
    return MatrixStateLoadResult::CORRUPT;
  }

  memcpy(&state, &storageBuffer, sizeof(state));
  return MatrixStateLoadResult::LOADED;
}

MatrixStateSaveResult matrix_state_storage_save(const MatrixStateV2 &state) {
  memcpy(&storageBuffer, &state, sizeof(storageBuffer));
  if (matrix_state_storage_validate(storageBuffer) !=
      MatrixStateValidationResult::VALID) {
    return MatrixStateSaveResult::INVALID_STATE;
  }

  if (!initializeMatrixPartition()) {
    return MatrixStateSaveResult::STORAGE_ERROR;
  }

  nvs_handle_t handle = 0;
  const esp_err_t openError = nvs_open_from_partition(
      MATRIX_NVS_PARTITION, MATRIX_NVS_NAMESPACE, NVS_READWRITE, &handle);
  if (openError != ESP_OK) {
    logNvsError("open for write", openError);
    return MatrixStateSaveResult::STORAGE_ERROR;
  }

  const esp_err_t writeError =
      nvs_set_blob(handle, MATRIX_NVS_KEY, &storageBuffer, sizeof(storageBuffer));
  const esp_err_t commitError =
      writeError == ESP_OK ? nvs_commit(handle) : ESP_OK;

  const ReadBlobResult readResult = readBlob(handle, storageBuffer);
  const bool verified =
      readResult == ReadBlobResult::OK &&
      matrix_state_storage_validate(storageBuffer) ==
          MatrixStateValidationResult::VALID &&
      memcmp(&storageBuffer, &state, sizeof(state)) == 0;
  nvs_close(handle);

  if (writeError != ESP_OK) {
    logNvsError("write", writeError);
    return MatrixStateSaveResult::STORAGE_ERROR;
  }
  if (commitError != ESP_OK) {
    logNvsError("commit", commitError);
    return MatrixStateSaveResult::STORAGE_ERROR;
  }

  if (verified) {
    return MatrixStateSaveResult::SAVED;
  }

  Serial.println("matrix_nvs readback verification failed; partition not erased");
  return MatrixStateSaveResult::VERIFY_FAILED;
}

bool matrix_state_storage_clear_saved_state() {
  if (!initializeMatrixPartition()) {
    return false;
  }

  nvs_handle_t handle = 0;
  const esp_err_t openError = nvs_open_from_partition(
      MATRIX_NVS_PARTITION, MATRIX_NVS_NAMESPACE, NVS_READWRITE, &handle);
  if (openError != ESP_OK) {
    logNvsError("open for clear", openError);
    return false;
  }

  const esp_err_t eraseError = nvs_erase_key(handle, MATRIX_NVS_KEY);
  if (eraseError == ESP_ERR_NVS_NOT_FOUND) {
    nvs_close(handle);
    return true;
  }
  if (eraseError != ESP_OK) {
    logNvsError("erase state", eraseError);
    nvs_close(handle);
    return false;
  }

  const esp_err_t commitError = nvs_commit(handle);
  nvs_close(handle);
  if (commitError != ESP_OK) {
    logNvsError("commit clear", commitError);
    return false;
  }
  return true;
}

bool matrix_state_v3_calculate_output_layout(
    const MatrixStateV3 &state,
    MatrixStateV3OutputLayout (&layout)[LED_OUTPUT_COUNT],
    size_t &totalLedCount) {
  MatrixStateV3OutputLayout calculatedLayout[LED_OUTPUT_COUNT] = {};
  size_t calculatedTotal = 0;

  for (size_t outputIndex = 0; outputIndex < LED_OUTPUT_COUNT;
       ++outputIndex) {
    const MatrixStateV3Output &output = state.outputs[outputIndex];
    if (output.enabled > 1 || output.topology > TOPOLOGY_MAX ||
        !hardware_config_led_output_supported(
            static_cast<LedProtocol>(output.ledProtocol),
            static_cast<LedColorOrder>(output.colorOrder))) {
      return false;
    }

    size_t length = 0;
    if (output.enabled == 0) {
      if (output.rows != 0 || output.cols != 0) {
        return false;
      }
    } else {
      // The division proves the product fits the per-output limit before the
      // multiplication is performed.
      if (!v3EnabledOutputDimensionsAreValid(output.rows, output.cols)) {
        return false;
      }

      const size_t rows = output.rows;
      const size_t cols = output.cols;
      length = rows * cols;
      if (!v3CanAppendOutputLength(calculatedTotal, length)) {
        return false;
      }
    }

    calculatedLayout[outputIndex].offset =
        static_cast<uint16_t>(calculatedTotal);
    calculatedLayout[outputIndex].length = static_cast<uint16_t>(length);
    calculatedTotal += length;
  }

  memcpy(layout, calculatedLayout, sizeof(calculatedLayout));
  totalLedCount = calculatedTotal;
  return true;
}

void matrix_state_storage_clear_v3_spatial_data(MatrixStateV3 &state) {
  memset(state.zoneMap, ZONE_ID_OFF, sizeof(state.zoneMap));
  memset(state.layers, 0, sizeof(state.layers));
}

void matrix_state_storage_set_v3_factory_defaults(MatrixStateV3 &state) {
  memset(&state, 0, sizeof(state));

  const HardwareConfig hardware = hardware_config_factory_defaults();
  state.magic = MATRIX_STATE_V3_MAGIC;
  state.formatVersion = MATRIX_STATE_V3_FORMAT_VERSION;
  state.structureSize = static_cast<uint16_t>(sizeof(state));
  state.activePortCount = hardware.activePortCount;
  state.portBrightness = DEFAULT_PORT_BRIGHTNESS;
  memcpy(state.waitingColor,
         FACTORY_STATUS_COLORS[MATRIX_STATE_LAYER_WAITING],
         sizeof(state.waitingColor));
  memcpy(state.chargingColor,
         FACTORY_STATUS_COLORS[MATRIX_STATE_LAYER_CHARGING],
         sizeof(state.chargingColor));
  memcpy(state.errorColor,
         FACTORY_STATUS_COLORS[MATRIX_STATE_LAYER_ERROR],
         sizeof(state.errorColor));

  for (size_t outputIndex = 0; outputIndex < LED_OUTPUT_COUNT;
       ++outputIndex) {
    MatrixStateV3Output &output = state.outputs[outputIndex];
    output.topology = 0;
    output.ledProtocol = static_cast<uint8_t>(hardware.ledProtocol);
    output.colorOrder = static_cast<uint8_t>(hardware.colorOrder);
    setV3FreeZoneDefaults(state.freeZones[outputIndex]);
  }

  state.outputs[0].rows = hardware.rows;
  state.outputs[0].cols = hardware.cols;
  state.outputs[0].enabled = 1;
  matrix_state_storage_clear_v3_spatial_data(state);
}

MatrixStateValidationResult matrix_state_storage_validate_v3(
    const MatrixStateV3 &state) {
  if (state.magic != MATRIX_STATE_V3_MAGIC ||
      state.formatVersion != MATRIX_STATE_V3_FORMAT_VERSION ||
      state.structureSize != sizeof(MatrixStateV3)) {
    return MatrixStateValidationResult::INCOMPATIBLE;
  }

  if (state.activePortCount < 1 ||
      state.activePortCount > MAX_PORT_COUNT ||
      !bytesAreZero(state.reservedHeader, sizeof(state.reservedHeader)) ||
      !bytesAreZero(state.reservedTail, sizeof(state.reservedTail))) {
    return MatrixStateValidationResult::INVALID;
  }

  MatrixStateV3OutputLayout layout[LED_OUTPUT_COUNT] = {};
  size_t totalLedCount = 0;
  if (!matrix_state_v3_calculate_output_layout(
          state, layout, totalLedCount)) {
    return MatrixStateValidationResult::INVALID;
  }

  for (size_t outputIndex = 0; outputIndex < LED_OUTPUT_COUNT;
       ++outputIndex) {
    for (size_t freeZoneIndex = 0; freeZoneIndex < FREE_ZONE_COUNT;
         ++freeZoneIndex) {
      if (!freeZoneIsValid(
              state.freeZones[outputIndex][freeZoneIndex], freeZoneIndex)) {
        return MatrixStateValidationResult::INVALID;
      }
    }
  }

  for (size_t pixel = 0; pixel < MAX_TOTAL_LED_COUNT; ++pixel) {
    const uint8_t zoneId = state.zoneMap[pixel];
    if (!zone_id_is_valid(zoneId) ||
        (pixel >= totalLedCount && zoneId != ZONE_ID_OFF)) {
      return MatrixStateValidationResult::INVALID;
    }

    if (pixel < totalLedCount && zone_is_port(zoneId) &&
        zoneId - ZONE_ID_PORT1 >= state.activePortCount) {
      return MatrixStateValidationResult::INVALID;
    }
  }

  for (size_t layerIndex = 0; layerIndex < MATRIX_STATE_V3_LAYER_COUNT;
       ++layerIndex) {
    if (!v3LayerIsValid(state.layers[layerIndex], totalLedCount)) {
      return MatrixStateValidationResult::INVALID;
    }
  }

  return MatrixStateValidationResult::VALID;
}
