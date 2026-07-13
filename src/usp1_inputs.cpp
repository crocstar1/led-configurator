#include "usp1_inputs.h"

static Usp1InputState inputState = {};

void usp1_inputs_setup() {
    for (uint8_t i = 0; i < USP1_DATA_INPUT_COUNT; i++) {
        pinMode(USP1_DATA_PINS[i], INPUT_PULLUP);
    }
    usp1_inputs_update();
}

void usp1_inputs_update() {
    for (uint8_t i = 0; i < USP1_DATA_INPUT_COUNT; i++) {
        const uint8_t level = digitalRead(USP1_DATA_PINS[i]);
        const bool active = USP1_INPUT_ACTIVE_LOW ? (level == LOW) : (level == HIGH);

        inputState.rawLevel[i] = level;
        inputState.active[i] = active;
    }
}

const Usp1InputState &usp1_inputs_get_state() {
    return inputState;
}
