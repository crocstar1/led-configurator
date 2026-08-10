#ifndef STATUS_MAPPER_H
#define STATUS_MAPPER_H

#include <Arduino.h>
#include "board_config.h"
#include "station_inputs.h"

enum PortStatus : uint8_t
{
    PORT_STATUS_WAITING = 0,
    PORT_STATUS_CHARGING,
    PORT_STATUS_ERROR,
    PORT_STATUS_DISABLED
};

struct PortInputMapping
{ // хранит для одного порта индексы charging/error входов, zoneId и enabled
    uint8_t chargingInput;
    uint8_t errorInput;
    uint8_t zoneId;
    bool enabled;
};

struct StatusMapperState
{
    uint8_t activePortCount;
    PortInputMapping ports[MAX_PORT_COUNT];
    PortStatus statuses[MAX_PORT_COUNT];
};

bool status_mapper_setup(uint8_t activePortCount);
void status_mapper_update(const StationInputState &inputs);
void status_mapper_get_snapshot(StatusMapperState &snapshot);
const char *status_mapper_status_to_string(PortStatus status);

#endif
