#ifndef STATUS_MAPPER_H
#define STATUS_MAPPER_H

#include <Arduino.h>
#include "usp1_board_config.h"
#include "usp1_inputs.h"

enum PortStatus : uint8_t {
    PORT_STATUS_WAITING = 0,
    PORT_STATUS_CHARGING,
    PORT_STATUS_ERROR,
    PORT_STATUS_DISABLED
};

struct PortInputMapping {
    uint8_t chargingInput;
    uint8_t errorInput;
    uint8_t zoneId;
    bool enabled;
};

struct StatusMapperState {
    uint8_t activePortCount;
    PortInputMapping ports[USP1_MAX_PORT_COUNT];
    PortStatus statuses[USP1_MAX_PORT_COUNT];
};

void status_mapper_setup();
void status_mapper_update(const Usp1InputState &inputs);
const StatusMapperState &status_mapper_get_state();
void status_mapper_set_active_port_count(uint8_t activePortCount);
bool status_mapper_configure_port(uint8_t portIndex, uint8_t chargingInput, uint8_t errorInput, uint8_t zoneId);
const char *status_mapper_status_to_string(PortStatus status);

#endif
