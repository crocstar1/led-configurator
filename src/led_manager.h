#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>

#include "board_config.h"
#include "matrix_state_storage.h"

#define LED_PIN PRIMARY_LED_PIN

// LED matrix power budget used by FastLED brightness scaling.
#define MAX_LED_VOLTS 12
#define MAX_LED_MILLIAMPS 3000

struct LedRuntimeState {
  bool mutexReady;
  bool taskStarted;
  bool loopFallback;
  uint64_t lastRefreshAgeMs;
};

void led_setup(const MatrixStateV2 &initialState);
void led_loop();
void led_get_runtime_state(LedRuntimeState &state);

// Snapshot/apply are the only access paths to the internal activeState.
// Both copy the complete state while holding ledMutex when it exists.
bool led_get_matrix_state_snapshot(MatrixStateV2 &snapshot);
bool led_apply_matrix_state_safe(const MatrixStateV2 &state);
bool led_prepare_for_restart();

#endif
