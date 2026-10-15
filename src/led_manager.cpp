#include "led_manager.h"

#include <FastLED.h>
#include <esp_timer.h>
#include <string.h>

#include "station_inputs.h"
#include "status_mapper.h"
#include "zone_model.h"

namespace {

static CRGB leds[MAX_TOTAL_LED_COUNT];
static MatrixStateV3 activeState;
static MatrixStateV3OutputLayout activeLayout[LED_OUTPUT_COUNT] = {};
static size_t activeTotalLedCount = 0;
static uint8_t activeEnabledOutputCount = 0;
static uint8_t registeredControllerCount = 0;
static bool activeStateReady = false;
static bool ledOutputStoppedForRestart = false;

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

bool calculateRuntimeLayout(
    const MatrixStateV3 &state,
    MatrixStateV3OutputLayout (&layout)[LED_OUTPUT_COUNT],
    size_t &totalLedCount,
    uint8_t &enabledOutputCount) {
  if (matrix_state_storage_validate_v3(state) !=
      MatrixStateValidationResult::VALID) {
    return false;
  }

  if (!matrix_state_v3_calculate_output_layout(state, layout, totalLedCount)) {
    return false;
  }

  enabledOutputCount = 0;
  for (size_t outputIndex = 0; outputIndex < LED_OUTPUT_COUNT;
       ++outputIndex) {
    if (layout[outputIndex].length > 0) {
      ++enabledOutputCount;
    }
  }
  return true;
}

template <uint8_t DATA_PIN>
bool registerLedControllerForPin(
    const MatrixStateV3Output &output,
    const MatrixStateV3OutputLayout &layout) {
  if (layout.length == 0 ||
      static_cast<size_t>(layout.offset) + layout.length >
          MAX_TOTAL_LED_COUNT) {
    return false;
  }

  const LedProtocol protocol = static_cast<LedProtocol>(output.ledProtocol);
  const LedColorOrder colorOrder =
      static_cast<LedColorOrder>(output.colorOrder);
  if (!hardware_config_led_output_supported(protocol, colorOrder)) {
    return false;
  }

  CRGB *const outputLeds = leds + layout.offset;
  const int outputLength = static_cast<int>(layout.length);
  switch (protocol) {
    case LedProtocol::WS2811_800K:
      switch (colorOrder) {
        case LedColorOrder::RGB:
          FastLED.addLeds<WS2811, DATA_PIN, RGB>(outputLeds, outputLength);
          return true;
        case LedColorOrder::RBG:
          FastLED.addLeds<WS2811, DATA_PIN, RBG>(outputLeds, outputLength);
          return true;
        case LedColorOrder::GRB:
          FastLED.addLeds<WS2811, DATA_PIN, GRB>(outputLeds, outputLength);
          return true;
        case LedColorOrder::GBR:
          FastLED.addLeds<WS2811, DATA_PIN, GBR>(outputLeds, outputLength);
          return true;
        case LedColorOrder::BRG:
          FastLED.addLeds<WS2811, DATA_PIN, BRG>(outputLeds, outputLength);
          return true;
        case LedColorOrder::BGR:
          FastLED.addLeds<WS2811, DATA_PIN, BGR>(outputLeds, outputLength);
          return true;
        default:
          return false;
      }
    default:
      return false;
  }
}

bool registerOutputController(size_t outputIndex,
                              const MatrixStateV3Output &output,
                              const MatrixStateV3OutputLayout &layout) {
  switch (outputIndex) {
    case 0:
      return registerLedControllerForPin<LED_OUTPUT_PINS[0]>(output, layout);
    case 1:
      return registerLedControllerForPin<LED_OUTPUT_PINS[1]>(output, layout);
    case 2:
      return registerLedControllerForPin<LED_OUTPUT_PINS[2]>(output, layout);
    case 3:
      return registerLedControllerForPin<LED_OUTPUT_PINS[3]>(output, layout);
    default:
      return false;
  }
}

bool registerEnabledControllers(const MatrixStateV3 &state) {
  registeredControllerCount = 0;
  for (size_t outputIndex = 0; outputIndex < LED_OUTPUT_COUNT;
       ++outputIndex) {
    if (activeLayout[outputIndex].length == 0) {
      continue;
    }
    if (!registerOutputController(outputIndex,
                                  state.outputs[outputIndex],
                                  activeLayout[outputIndex])) {
      return false;
    }
    ++registeredControllerCount;
  }
  return registeredControllerCount == activeEnabledOutputCount;
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

bool stopLedOutputForRestart() {
  if (ledOutputStoppedForRestart) {
    return true;
  }
  if (registeredControllerCount == 0) {
    ledOutputStoppedForRestart = true;
    return true;
  }
  if (!activeStateReady || activeTotalLedCount == 0 ||
      activeTotalLedCount > MAX_TOTAL_LED_COUNT) {
    return false;
  }

  ledOutputStoppedForRestart = true;
  fill_solid(leds, activeTotalLedCount, CRGB::Black);
  FastLED.show();
  return true;
}

bool startupShowColor(const CRGB &color) {
  if (registeredControllerCount == 0) {
    return true;
  }
  if (startupErrorActive()) {
    return false;
  }

  fill_solid(leds, activeTotalLedCount, color);
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

// Preserve the existing topology meanings for arbitrary rows/cols.
int logicalToPhysicalIndex(uint16_t x,
                           uint16_t y,
                           const MatrixStateV3Output &output,
                           size_t outputLedCount) {
  if (x >= output.cols || y >= output.rows) {
    return -1;
  }

  uint16_t tx = x;
  uint16_t ty = y;
  size_t physicalIndex = 0;

  switch (output.topology) {
    case 0:  // Right-bottom start, vertical snake.
      tx = output.cols - 1U - x;
      physicalIndex = static_cast<size_t>(tx) * output.rows +
                      ((tx % 2U) == 0 ? ty : output.rows - 1U - ty);
      break;
    case 1:  // Left-bottom start, vertical snake.
      physicalIndex = static_cast<size_t>(tx) * output.rows +
                      ((tx % 2U) == 0 ? ty : output.rows - 1U - ty);
      break;
    case 2:  // Left-top start, horizontal snake.
      ty = output.rows - 1U - y;
      physicalIndex = static_cast<size_t>(ty) * output.cols +
                      ((ty % 2U) == 0 ? tx : output.cols - 1U - tx);
      break;
    case 3:  // Right-top start, horizontal snake.
      tx = output.cols - 1U - x;
      ty = output.rows - 1U - y;
      physicalIndex = static_cast<size_t>(ty) * output.cols +
                      ((ty % 2U) == 0 ? tx : output.cols - 1U - tx);
      break;
    case 4:  // Right-top start, vertical snake.
      tx = output.cols - 1U - x;
      physicalIndex = static_cast<size_t>(tx) * output.rows +
                      ((tx % 2U) == 0 ? output.rows - 1U - ty : ty);
      break;
    case 5:  // Left-top start, vertical snake.
      physicalIndex = static_cast<size_t>(tx) * output.rows +
                      ((tx % 2U) == 0 ? output.rows - 1U - ty : ty);
      break;
    case 6:  // Left-bottom start, horizontal snake.
      physicalIndex = static_cast<size_t>(ty) * output.cols +
                      ((ty % 2U) == 0 ? tx : output.cols - 1U - tx);
      break;
    case 7:  // Right-bottom start, horizontal snake.
      tx = output.cols - 1U - x;
      physicalIndex = static_cast<size_t>(ty) * output.cols +
                      ((ty % 2U) == 0 ? tx : output.cols - 1U - tx);
      break;
    default:
      return -1;
  }

  return physicalIndex < outputLedCount
             ? static_cast<int>(physicalIndex)
             : -1;
}

bool storedLayerColor(const MatrixStateV3StoredLayer &layer,
                      size_t logicalIndex,
                      CRGB &color) {
  if (logicalIndex >= activeTotalLedCount ||
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

size_t statusLayerIndex(PortStatus status) {
  if (status == PORT_STATUS_WAITING) {
    return MATRIX_STATE_LAYER_WAITING;
  }
  if (status == PORT_STATUS_CHARGING) {
    return MATRIX_STATE_LAYER_CHARGING;
  }
  if (status == PORT_STATUS_ERROR) {
    return MATRIX_STATE_LAYER_ERROR;
  }
  return MATRIX_STATE_LAYER_COUNT;
}

const MatrixStateStoredFreeZone *freeZoneConfig(size_t outputIndex,
                                                uint8_t zoneId,
                                                size_t &zoneIndex) {
  const int index = free_zone_index(zoneId);
  if (outputIndex >= LED_OUTPUT_COUNT || index < 0) {
    return nullptr;
  }

  zoneIndex = static_cast<size_t>(index);
  return &activeState.freeZones[outputIndex][zoneIndex];
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
  const size_t layerIndex = statusLayerIndex(status);
  if (layerIndex >= MATRIX_STATE_STATUS_LIGHT_MODE_COUNT) {
    return CRGB::Black;
  }
  const uint8_t brightness = activeState.statusBrightness[layerIndex];
  if (brightness == 0) {
    return CRGB::Black;
  }
  const MatrixStateV3StoredLayer &layer = activeState.layers[layerIndex];
  if (!storedLayerColor(layer, logicalIndex, color)) {
    if (status == PORT_STATUS_WAITING) {
      color = CRGB(activeState.waitingColor[0],
                   activeState.waitingColor[1],
                   activeState.waitingColor[2]);
    } else if (status == PORT_STATUS_CHARGING) {
      color = CRGB(activeState.chargingColor[0],
                   activeState.chargingColor[1],
                   activeState.chargingColor[2]);
    } else {
      color = CRGB(activeState.errorColor[0],
                   activeState.errorColor[1],
                   activeState.errorColor[2]);
    }
  }
  color.nscale8_video(brightness);

  const StatusLightMode lightMode = matrix_state_resolve_status_light_mode(
      layerIndex,
      activeState.statusLightModes[layerIndex]);
  if (lightMode == StatusLightMode::PULSE_MODE) {
    color.nscale8_video(pulseBrightness);
  }
  return color;
}

CRGB renderFreeZonePixel(size_t outputIndex,
                         size_t globalLogicalIndex,
                         size_t localLogicalIndex,
                         uint8_t zoneId,
                         uint8_t rainbowHue) {
  size_t zoneIndex = 0;
  const MatrixStateStoredFreeZone *config =
      freeZoneConfig(outputIndex, zoneId, zoneIndex);
  if (config == nullptr || config->enabled == 0) {
    return CRGB::Black;
  }

  if (config->mode == FREE_ZONE_RAINBOW) {
    return CHSV(rainbowHue + static_cast<uint8_t>(localLogicalIndex * 4U),
                255,
                config->brightness);
  }

  CRGB color;
  if (config->mode == FREE_ZONE_CUSTOM) {
    const MatrixStateV3StoredLayer &layer =
        activeState.layers[MATRIX_STATE_LAYER_LOGO + zoneIndex];
    if (!storedLayerColor(layer, globalLogicalIndex, color)) {
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
  if (registeredControllerCount == 0 || activeTotalLedCount == 0) {
    return;
  }

  static uint8_t rainbowHue = 0;
  ++rainbowHue;

  uint8_t pulseBrightness = static_cast<uint8_t>(220.0f * breatheScale);
  if (pulseBrightness < 40) {
    pulseBrightness = 40;
  }

  StatusMapperState ports;
  status_mapper_get_snapshot(ports);
  fill_solid(leds, activeTotalLedCount, CRGB::Black);

  for (size_t outputIndex = 0; outputIndex < LED_OUTPUT_COUNT;
       ++outputIndex) {
    const MatrixStateV3Output &output = activeState.outputs[outputIndex];
    const MatrixStateV3OutputLayout &layout = activeLayout[outputIndex];
    if (layout.length == 0) {
      continue;
    }

    for (size_t localIndex = 0; localIndex < layout.length; ++localIndex) {
      const uint16_t x = localIndex % output.cols;
      const uint16_t y = localIndex / output.cols;
      const size_t globalIndex = layout.offset + localIndex;
      const uint8_t zoneId = activeState.zoneMap[globalIndex];

      CRGB color = CRGB::Black;
      if (zone_is_port(zoneId)) {
        color = renderPortPixel(globalIndex, zoneId, ports, pulseBrightness);
      } else if (zone_is_free(zoneId)) {
        // A hidden free-layer pixel is visible only while zoneMap selects it.
        color = renderFreeZonePixel(outputIndex,
                                    globalIndex,
                                    localIndex,
                                    zoneId,
                                    rainbowHue);
      }

      const int localPhysicalIndex =
          logicalToPhysicalIndex(x, y, output, layout.length);
      if (localPhysicalIndex >= 0) {
        const size_t bufferIndex =
            layout.offset + static_cast<size_t>(localPhysicalIndex);
        if (bufferIndex < activeTotalLedCount) {
          leds[bufferIndex] = color;
        }
      }
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
      if (!ledOutputStoppedForRestart) {
        updateStep(millis());
      }
      xSemaphoreGive(ledMutex);
    }
    vTaskDelay(15 / portTICK_PERIOD_MS);
  }
}

}  // namespace

void led_setup(const MatrixStateV3 &initialState) {
  if (!calculateRuntimeLayout(initialState,
                              activeLayout,
                              activeTotalLedCount,
                              activeEnabledOutputCount)) {
    Serial.println(
        "Initial MatrixStateV3 is invalid; LED runtime not started");
    return;
  }
  memcpy(&activeState, &initialState, sizeof(activeState));
  activeStateReady = true;

  if (activeEnabledOutputCount == 0) {
    Serial.println("All LED outputs are disabled; LED task not started");
    return;
  }

  if (!registerEnabledControllers(activeState)) {
    Serial.println("LED controller registration failed; LED task not started");
    return;
  }

  ledMutex = xSemaphoreCreateMutex();
  ledMutexReady = ledMutex != nullptr;
  if (!ledMutexReady) {
    Serial.println("LED mutex creation failed; LED task will not start");
  }

  fill_solid(leds, activeTotalLedCount, CRGB::Black);
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
  if (!activeStateReady || registeredControllerCount == 0 || !ledLoopFallback ||
      ledOutputStoppedForRestart) {
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
    if (!ledOutputStoppedForRestart) {
      updateStep(now);
    }
    xSemaphoreGive(ledMutex);
    return;
  }

  updateStep(now);
}

void led_get_runtime_state(LedRuntimeState &state) {
  state.mutexReady = ledMutexReady;
  state.taskStarted = ledTaskStarted;
  state.loopFallback = ledLoopFallback;
  state.enabledOutputCount = activeEnabledOutputCount;
  state.totalLedCount = static_cast<uint16_t>(activeTotalLedCount);
  for (size_t outputIndex = 0; outputIndex < LED_OUTPUT_COUNT;
       outputIndex++) {
    state.outputLedCount[outputIndex] = activeLayout[outputIndex].length;
  }

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

bool led_get_matrix_state_snapshot(MatrixStateV3 &snapshot) {
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

bool led_apply_matrix_state_safe(const MatrixStateV3 &state) {
  MatrixStateV3OutputLayout nextLayout[LED_OUTPUT_COUNT] = {};
  size_t nextTotalLedCount = 0;
  uint8_t nextEnabledOutputCount = 0;
  if (ledOutputStoppedForRestart || !activeStateReady ||
      !calculateRuntimeLayout(state,
                              nextLayout,
                              nextTotalLedCount,
                              nextEnabledOutputCount) ||
      nextTotalLedCount != activeTotalLedCount ||
      nextEnabledOutputCount != activeEnabledOutputCount ||
      state.activePortCount != activeState.activePortCount) {
    return false;
  }

  // Controller registration and geometry are fixed until restart. Topology is
  // deliberately excluded: it changes only logical-to-physical mapping and is
  // safe to apply immediately.
  for (size_t outputIndex = 0; outputIndex < LED_OUTPUT_COUNT;
       ++outputIndex) {
    const MatrixStateV3Output &next = state.outputs[outputIndex];
    const MatrixStateV3Output &current = activeState.outputs[outputIndex];
    if (next.enabled != current.enabled || next.rows != current.rows ||
        next.cols != current.cols ||
        next.ledProtocol != current.ledProtocol ||
        next.colorOrder != current.colorOrder ||
        nextLayout[outputIndex].offset != activeLayout[outputIndex].offset ||
        nextLayout[outputIndex].length != activeLayout[outputIndex].length) {
      return false;
    }
  }

  if (!ledMutexReady) {
    memcpy(&activeState, &state, sizeof(activeState));
    if (registeredControllerCount > 0) {
      refreshInternal();
    }
    return true;
  }

  if (xSemaphoreTake(
          ledMutex, pdMS_TO_TICKS(LED_STATE_MUTEX_TIMEOUT_MS)) != pdTRUE) {
    return false;
  }
  if (ledOutputStoppedForRestart) {
    xSemaphoreGive(ledMutex);
    return false;
  }
  memcpy(&activeState, &state, sizeof(activeState));
  refreshInternal();
  xSemaphoreGive(ledMutex);
  return true;
}

bool led_prepare_for_restart() {
  if (!ledMutexReady) {
    return stopLedOutputForRestart();
  }

  if (xSemaphoreTake(
          ledMutex, pdMS_TO_TICKS(LED_STATE_MUTEX_TIMEOUT_MS)) != pdTRUE) {
    return false;
  }
  const bool prepared = stopLedOutputForRestart();
  xSemaphoreGive(ledMutex);
  return prepared;
}
