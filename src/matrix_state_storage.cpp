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

static_assert(MATRIX_STATE_LAYER_COUNT == 7,
              "The stored layer order must contain seven layers");

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
      DEFAULT_ACTIVE_PORT_COUNT,
      LedProtocol::WS2811_800K,
      LedColorOrder::BRG,
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

}  // namespace

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

  state.waitingColor[0] = 0;
  state.waitingColor[1] = 255;
  state.waitingColor[2] = 0;
  state.chargingColor[0] = 0;
  state.chargingColor[1] = 80;
  state.chargingColor[2] = 255;
  state.errorColor[0] = 255;
  state.errorColor[1] = 0;
  state.errorColor[2] = 0;

  setFreeZoneDefaults(state);

  size_t activeLedCount = 0;
  if (!calculateActiveLedCount(state, activeLedCount)) {
    return;
  }

  // Row-major 12x8 defaults split at logical row 4: Port 1 occupies y=0..3,
  // Port 2 occupies y=4..7. The same formula remains deterministic for other
  // factory dimensions and guarantees pixels for every active port.
  for (size_t pixel = 0; pixel < activeLedCount; ++pixel) {
    const uint8_t portOffset = static_cast<uint8_t>(
        (pixel * DEFAULT_ACTIVE_PORT_COUNT) / activeLedCount);
    state.zoneMap[pixel] = ZONE_ID_PORT1 + portOffset;
  }
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

  bool activePortSeen[MAX_PORT_COUNT] = {};
  for (size_t pixel = 0; pixel < activeLedCount; ++pixel) {
    const uint8_t zoneId = state.zoneMap[pixel];
    if (!zone_is_port(zoneId)) {
      continue;
    }

    const uint8_t portIndex = zoneId - ZONE_ID_PORT1;
    if (portIndex >= DEFAULT_ACTIVE_PORT_COUNT) {
      return MatrixStateValidationResult::INVALID;
    }
    activePortSeen[portIndex] = true;
  }
  for (size_t port = 0; port < DEFAULT_ACTIVE_PORT_COUNT; ++port) {
    if (!activePortSeen[port]) {
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
