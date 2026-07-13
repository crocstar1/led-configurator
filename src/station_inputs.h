#ifndef STATION_INPUTS_H
#define STATION_INPUTS_H

#include <Arduino.h>
#include "board_config.h"

struct StationInputState {
    bool active[BOARD_INPUT_COUNT];
    uint8_t rawLevel[BOARD_INPUT_COUNT];
};

void station_inputs_setup();
void station_inputs_update();
const StationInputState &station_inputs_get_state();

#endif
