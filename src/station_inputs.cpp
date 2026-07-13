#include "station_inputs.h"

static StationInputState inputState = {};

void station_inputs_setup() {
    for (uint8_t i = 0; i < BOARD_INPUT_COUNT; i++) {
        pinMode(BOARD_INPUT_PINS[i], INPUT_PULLUP);
    }
    station_inputs_update();
}

void station_inputs_update() {
    for (uint8_t i = 0; i < BOARD_INPUT_COUNT; i++) {
        const uint8_t level = digitalRead(BOARD_INPUT_PINS[i]);
        const bool active = BOARD_INPUT_ACTIVE_LOW ? (level == LOW) : (level == HIGH);

        inputState.rawLevel[i] = level;
        inputState.active[i] = active;
    }
}

const StationInputState &station_inputs_get_state() {
    return inputState;
}
