#ifndef USP1_INPUTS_H
#define USP1_INPUTS_H

#include <Arduino.h>
#include "usp1_board_config.h"

struct Usp1InputState {
    bool active[USP1_DATA_INPUT_COUNT];
    uint8_t rawLevel[USP1_DATA_INPUT_COUNT];
};

void usp1_inputs_setup();
void usp1_inputs_update();
const Usp1InputState &usp1_inputs_get_state();

#endif
