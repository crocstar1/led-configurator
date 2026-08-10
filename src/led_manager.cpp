#include "led_manager.h"

#include <FastLED.h>
#include <esp_timer.h>
#include <string.h>

#include "station_inputs.h"
#include "status_mapper.h"
#include "zone_model.h"

namespace {

static CRGB leds[MAX_LED_COUNT];
static MatrixStateV2 activeState;
static size_t activeLedCount = 0;
static bool activeStateReady = false;

static float breatheScale = 0.2f;
static bool breatheDirectionUp = true;
static unsigned long lastBreatheTime = 0;

static SemaphoreHandle_t ledMutex = nullptr;
static bool ledMutexReady = false;
static bool ledTaskStarted = false;
static bool ledLoopFallback = false;
static uint64_t lastLedRefreshMs = 0;
static portMUX_TYPE ledRuntimeMux = portMUX_INITIALIZER_UNLOCKED;

static constexpr uint16_t STARTUP_COLOR_MS = 180;
static constexpr uint16_t STARTUP_ERROR_POLL_MS = 20;
static constexpr uint32_t LED_STATE_MUTEX_TIMEOUT_MS = 50;

bool calculateActiveLedCount(const MatrixStateV2 &state, size_t &ledCount) {
  if (matrix_state_storage_validate(state) !=
      MatrixStateValidationResult::VALID) {
    return false;
  }

  const size_t rows = state.rows;
  const size_t cols = state.cols;
  if (cols == 0 || rows > MAX_LED_COUNT / cols) {
    return false;
  }

  ledCount = rows * cols;
  return ledCount > 0 && ledCount <= MAX_LED_COUNT;
}

const char *loadResultName(MatrixStateLoadResult result) {
  switch (result) {
    case MatrixStateLoadResult::LOADED:
      return "LOADED";
    case MatrixStateLoadResult::NOT_FOUND:
      return "NOT_FOUND";
    case MatrixStateLoadResult::INCOMPATIBLE:
      return "INCOMPATIBLE";
    case MatrixStateLoadResult::CORRUPT:
      return "CORRUPT";
    case MatrixStateLoadResult::STORAGE_ERROR:
    default:
      return "STORAGE_ERROR";
  }
}

bool startupErrorActive() {
  station_inputs_update();
  StationInputState inputs;
  station_inputs_get_snapshot(inputs);
  status_mapper_update(inputs);

  StatusMapperState state;
  status_mapper_get_snapshot(state);
  const uint8_t portCount = state.activePortCount > MAX_PORT_COUNT
                                ? MAX_PORT_COUNT
                                : state.activePortCount;
  for (uint8_t port = 0; port < portCount; ++port) {
    if (state.statuses[port] == PORT_STATUS_ERROR) {
      return true;
    }
  }
  return false;
}

bool startupShowColor(const CRGB &color) {
  if (startupErrorActive()) {
    return false;
  }

  fill_solid(leds, activeLedCount, color);
  FastLED.show();

  const unsigned long startedAt = millis();
  while (millis() - startedAt < STARTUP_COLOR_MS) {
    delay(STARTUP_ERROR_POLL_MS);
    if (startupErrorActive()) {
      return false;
    }
  }
  return true;
}

void runStartupAnimation() {
  if (!startupShowColor(CRGB::Red)) return;
  if (!startupShowColor(CRGB::Blue)) return;
  startupShowColor(CRGB::Green);
}

// Preserve the four existing topology meanings for arbitrary rows/cols.
int logicalToPhysicalIndex(uint16_t x,
                           uint16_t y,
                           const MatrixStateV2 &state) {
  if (x >= state.cols || y >= state.rows) {
    return -1;
  }

  uint16_t tx = x;
  uint16_t ty = y;
  size_t physicalIndex = 0;

  switch (state.topology) {
    case 0:  // Right-bottom start, vertical snake.
      tx = state.cols - 1U - x;
      physicalIndex = static_cast<size_t>(tx) * state.rows +
                      ((tx % 2U) == 0 ? ty : state.rows - 1U - ty);
      break;
    case 1:  // Left-bottom start, vertical snake.
      physicalIndex = static_cast<size_t>(tx) * state.rows +
                      ((tx % 2U) == 0 ? ty : state.rows - 1U - ty);
      break;
    case 2:  // Left-top start, horizontal snake.
      ty = state.rows - 1U - y;
      physicalIndex = static_cast<size_t>(ty) * state.cols +
                      ((ty % 2U) == 0 ? tx : state.cols - 1U - tx);
      break;
    case 3:  // Right-top start, horizontal snake.
      tx = state.cols - 1U - x;
      ty = state.rows - 1U - y;
      physicalIndex = static_cast<size_t>(ty) * state.cols +
                      ((ty % 2U) == 0 ? tx : state.cols - 1U - tx);
      break;
    default:
      return -1;
  }

  return physicalIndex < activeLedCount ? static_cast<int>(physicalIndex) : -1;
}

bool storedLayerColor(const MatrixStateStoredLayer &layer,
                      size_t logicalIndex,
                      CRGB &color) {
  if (logicalIndex >= activeLedCount ||
      (layer.present[logicalIndex / 8U] &
       (1U << (logicalIndex % 8U))) == 0) {
    return false;
  }

  const size_t rgbOffset = logicalIndex * 3U;
  color = CRGB(layer.rgb[rgbOffset],
               layer.rgb[rgbOffset + 1U],
               layer.rgb[rgbOffset + 2U]);
  return true;
}

const MatrixStateStoredLayer *statusLayer(PortStatus status) {
  if (status == PORT_STATUS_WAITING) {
    return &activeState.layers[MATRIX_STATE_LAYER_WAITING];
  }
  if (status == PORT_STATUS_CHARGING) {
    return &activeState.layers[MATRIX_STATE_LAYER_CHARGING];
  }
  if (status == PORT_STATUS_ERROR) {
    return &activeState.layers[MATRIX_STATE_LAYER_ERROR];
  }
  return nullptr;
}

const MatrixStateStoredFreeZone *freeZoneConfig(uint8_t zoneId,
                                                size_t &zoneIndex) {
  const int index = free_zone_index(zoneId);
  if (index < 0) {
    return nullptr;
  }

  zoneIndex = static_cast<size_t>(index);
  return &activeState.freeZones[zoneIndex];
}

CRGB visibleErrorColor(CRGB preferred) {
  if (static_cast<uint16_t>(preferred.r) + preferred.g + preferred.b < 40) {
    return CRGB(255, 0, 0);
  }
  return preferred;
}

CRGB renderPortPixel(size_t logicalIndex,
                     uint8_t zoneId,
                     const StatusMapperState &ports,
                     uint8_t pulseBrightness) {
  const uint8_t portIndex = zoneId - ZONE_ID_PORT1;
  const PortStatus status = portIndex < MAX_PORT_COUNT
                                ? ports.statuses[portIndex]
                                : PORT_STATUS_DISABLED;
  if (status == PORT_STATUS_DISABLED) {
    return CRGB::Black;
  }

  CRGB color;
  const MatrixStateStoredLayer *layer = statusLayer(status);
  if (layer != nullptr && storedLayerColor(*layer, logicalIndex, color)) {
    color.nscale8_video(activeState.portBrightness);
  } else if (status == PORT_STATUS_WAITING) {
    color = CRGB(activeState.waitingColor[0],
                 activeState.waitingColor[1],
                 activeState.waitingColor[2]);
    color.nscale8_video(activeState.portBrightness);
  } else if (status == PORT_STATUS_CHARGING) {
    color = CRGB(activeState.chargingColor[0],
                 activeState.chargingColor[1],
                 activeState.chargingColor[2]);
    color.nscale8_video(activeState.portBrightness);
  } else {
    color = CRGB(activeState.errorColor[0],
                 activeState.errorColor[1],
                 activeState.errorColor[2]);
    color.nscale8_video(activeState.portBrightness);
  }

  if (status == PORT_STATUS_ERROR) {
    color = visibleErrorColor(color);
    return ((millis() / 500U) % 2U) == 0 ? color : CRGB::Black;
  }
  if (status == PORT_STATUS_WAITING) {
    color.nscale8_video(pulseBrightness);
  }
  return color;
}

CRGB renderFreeZonePixel(size_t logicalIndex,
                         uint8_t zoneId,
                         uint8_t rainbowHue) {
  size_t zoneIndex = 0;
  const MatrixStateStoredFreeZone *config = freeZoneConfig(zoneId, zoneIndex);
  if (config == nullptr || config->enabled == 0) {
    return CRGB::Black;
  }

  if (config->mode == FREE_ZONE_RAINBOW) {
    return CHSV(rainbowHue + static_cast<uint8_t>(logicalIndex * 4U),
                255,
                config->brightness);
  }

  CRGB color;
  if (config->mode == FREE_ZONE_CUSTOM) {
    const MatrixStateStoredLayer &layer =
        activeState.layers[MATRIX_STATE_LAYER_LOGO + zoneIndex];
    if (!storedLayerColor(layer, logicalIndex, color)) {
      return CRGB::Black;
    }
  } else {
    color = CRGB(config->staticColor[0],
                 config->staticColor[1],
                 config->staticColor[2]);
  }

  color.nscale8_video(config->brightness);
  return color;
}

void refreshInternal() {
  static uint8_t rainbowHue = 0;
  ++rainbowHue;

  uint8_t pulseBrightness = static_cast<uint8_t>(220.0f * breatheScale);
  if (pulseBrightness < 40) {
    pulseBrightness = 40;
  }

  StatusMapperState ports;
  status_mapper_get_snapshot(ports);
  fill_solid(leds, activeLedCount, CRGB::Black);

  for (size_t logicalIndex = 0; logicalIndex < activeLedCount;
       ++logicalIndex) {
    const uint16_t x = logicalIndex % activeState.cols;
    const uint16_t y = logicalIndex / activeState.cols;
    const uint8_t zoneId = activeState.zoneMap[logicalIndex];

    CRGB color = CRGB::Black;
    if (zone_is_port(zoneId)) {
      color = renderPortPixel(logicalIndex, zoneId, ports, pulseBrightness);
    } else if (zone_is_free(zoneId)) {
      // A hidden free-layer pixel is visible only while zoneMap selects it.
      color = renderFreeZonePixel(logicalIndex, zoneId, rainbowHue);
    }

    const int physicalIndex = logicalToPhysicalIndex(x, y, activeState);
    if (physicalIndex >= 0) {
      leds[physicalIndex] = color;
    }
  }

  FastLED.show();

  const uint64_t refreshedAt =
      static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;
  portENTER_CRITICAL(&ledRuntimeMux);
  lastLedRefreshMs = refreshedAt;
  portEXIT_CRITICAL(&ledRuntimeMux);
}

void updateStep(unsigned long now) {
  if (now - lastBreatheTime <= 25) {
    return;
  }

  lastBreatheTime = now;
  if (breatheDirectionUp) {
    breatheScale += 0.012f;
    if (breatheScale >= 1.0f) breatheDirectionUp = false;
  } else {
    breatheScale -= 0.012f;
    if (breatheScale <= 0.15f) breatheDirectionUp = true;
  }
  refreshInternal();
}

void ledTaskWorker(void *) {
  while (true) {
    if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      updateStep(millis());
      xSemaphoreGive(ledMutex);
    }
    vTaskDelay(15 / portTICK_PERIOD_MS);
  }
}

}  // namespace

void led_setup() {
  matrix_state_storage_set_factory_defaults(activeState);
  const MatrixStateLoadResult loadResult =
      matrix_state_storage_load(activeState);
  if (loadResult == MatrixStateLoadResult::LOADED) {
    Serial.println("Matrix state V2 loaded");
  } else {
    Serial.printf("Matrix state V2 %s; using factory defaults without saving\n",
                  loadResultName(loadResult));
  }

  if (!calculateActiveLedCount(activeState, activeLedCount)) {
    Serial.println("Factory matrix state is invalid; LED runtime not started");
    return;
  }
  activeStateReady = true;

  ledMutex = xSemaphoreCreateMutex();
  ledMutexReady = ledMutex != nullptr;
  if (!ledMutexReady) {
    Serial.println("LED mutex creation failed; LED task will not start");
  }

  FastLED.addLeds<WS2811, LED_PIN, BRG>(
      leds, static_cast<int>(activeLedCount));
  FastLED.setMaxPowerInVoltsAndMilliamps(MAX_LED_VOLTS,
                                         MAX_LED_MILLIAMPS);

  fill_solid(leds, activeLedCount, CRGB::Black);
  FastLED.show();
  runStartupAnimation();
  refreshInternal();

  BaseType_t taskResult = pdFAIL;
  if (ledMutexReady) {
    taskResult = xTaskCreatePinnedToCore(
        ledTaskWorker, "LED_Task", 4096, nullptr, 1, nullptr, 1);
  }

  ledTaskStarted = taskResult == pdPASS;
  ledLoopFallback = !ledTaskStarted;
  if (ledTaskStarted) {
    Serial.println("LED task started normally");
  } else {
    Serial.println("LED task not started");
    Serial.println("LED loop fallback enabled");
  }
}

void led_loop() {
  if (!activeStateReady || !ledLoopFallback) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastBreatheTime <= 25) {
    return;
  }

  if (ledMutexReady) {
    if (xSemaphoreTake(ledMutex, 0) != pdTRUE) {
      return;
    }
    updateStep(now);
    xSemaphoreGive(ledMutex);
    return;
  }

  updateStep(now);
}

void led_get_runtime_state(LedRuntimeState &state) {
  state.mutexReady = ledMutexReady;
  state.taskStarted = ledTaskStarted;
  state.loopFallback = ledLoopFallback;

  uint64_t lastRefreshMs;
  portENTER_CRITICAL(&ledRuntimeMux);
  lastRefreshMs = lastLedRefreshMs;
  portEXIT_CRITICAL(&ledRuntimeMux);

  const uint64_t nowMs =
      static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;
  state.lastRefreshAgeMs = lastRefreshMs > 0 && nowMs >= lastRefreshMs
                               ? nowMs - lastRefreshMs
                               : 0;
}

bool led_get_matrix_state_snapshot(MatrixStateV2 &snapshot) {
  if (!activeStateReady) {
    return false;
  }

  if (!ledMutexReady) {
    memcpy(&snapshot, &activeState, sizeof(snapshot));
    return true;
  }

  if (xSemaphoreTake(
          ledMutex, pdMS_TO_TICKS(LED_STATE_MUTEX_TIMEOUT_MS)) != pdTRUE) {
    return false;
  }
  memcpy(&snapshot, &activeState, sizeof(snapshot));
  xSemaphoreGive(ledMutex);
  return true;
}

bool led_apply_matrix_state_safe(const MatrixStateV2 &state) {
  size_t nextLedCount = 0;
  if (!activeStateReady || !calculateActiveLedCount(state, nextLedCount) ||
      nextLedCount != activeLedCount || state.rows != activeState.rows ||
      state.cols != activeState.cols) {
    return false;
  }

  if (!ledMutexReady) {
    memcpy(&activeState, &state, sizeof(activeState));
    refreshInternal();
    return true;
  }

  if (xSemaphoreTake(
          ledMutex, pdMS_TO_TICKS(LED_STATE_MUTEX_TIMEOUT_MS)) != pdTRUE) {
    return false;
  }
  memcpy(&activeState, &state, sizeof(activeState));
  refreshInternal();
  xSemaphoreGive(ledMutex);
  return true;
}
