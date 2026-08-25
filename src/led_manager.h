#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>

#include "board_config.h"
#include "matrix_state_storage.h"

struct LedRuntimeState {
  bool mutexReady;
  bool taskStarted;
  bool loopFallback;
  uint8_t enabledOutputCount;
  uint16_t totalLedCount;
  uint16_t outputLedCount[LED_OUTPUT_COUNT];
  uint64_t lastRefreshAgeMs;
};

void led_setup(const MatrixStateV3 &initialState);
void led_loop();
void led_get_runtime_state(LedRuntimeState &state);

// Snapshot/apply are the only access paths to the internal activeState.
// Both copy the complete state while holding ledMutex when it exists.
bool led_get_matrix_state_snapshot(MatrixStateV3 &snapshot);
bool led_apply_matrix_state_safe(const MatrixStateV3 &state);
bool led_prepare_for_restart();

#endif
