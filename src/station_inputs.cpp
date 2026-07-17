#include "station_inputs.h"

static StationInputState inputState = {};
static portMUX_TYPE inputStateMux = portMUX_INITIALIZER_UNLOCKED;

void station_inputs_setup() {
    for (uint8_t i = 0; i < BOARD_INPUT_COUNT; i++) {
        pinMode(BOARD_INPUT_PINS[i], INPUT_PULLUP);
    }
    station_inputs_update();
}

void station_inputs_update() {
    StationInputState nextState = {};

    for (uint8_t i = 0; i < BOARD_INPUT_COUNT; i++) {
        const uint8_t level = digitalRead(BOARD_INPUT_PINS[i]);
        const bool active = BOARD_INPUT_ACTIVE_LOW ? (level == LOW) : (level == HIGH);

        nextState.rawLevel[i] = level;
        nextState.active[i] = active;
    }

    portENTER_CRITICAL(&inputStateMux);
    inputState = nextState;
    portEXIT_CRITICAL(&inputStateMux);
}

void station_inputs_get_snapshot(StationInputState &snapshot) {
    portENTER_CRITICAL(&inputStateMux);
    snapshot = inputState;
    portEXIT_CRITICAL(&inputStateMux);
}
