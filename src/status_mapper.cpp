#include "status_mapper.h"

static StatusMapperState mapperState = {};
static portMUX_TYPE mapperStateMux = portMUX_INITIALIZER_UNLOCKED;

static_assert(BOARD_INPUT_COUNT >= MAX_PORT_COUNT * 2,
              "Each port requires charging and error inputs");

bool status_mapper_setup(uint8_t activePortCount) {
    if (activePortCount < 1 || activePortCount > MAX_PORT_COUNT) {
        return false;
    }

    StatusMapperState nextState = {};
    nextState.activePortCount = activePortCount;

    for (uint8_t i = 0; i < MAX_PORT_COUNT; i++) {
        nextState.ports[i].chargingInput = i * 2;
        nextState.ports[i].errorInput = (i * 2) + 1;
        nextState.ports[i].zoneId = i + 1;
        nextState.ports[i].enabled = (i < nextState.activePortCount);
        nextState.statuses[i] = nextState.ports[i].enabled
            ? PORT_STATUS_WAITING
            : PORT_STATUS_DISABLED;
    }

    portENTER_CRITICAL(&mapperStateMux);
    mapperState = nextState;
    portEXIT_CRITICAL(&mapperStateMux);
    return true;
}

void status_mapper_update(const StationInputState &inputs) {
    StatusMapperState nextState;
    status_mapper_get_snapshot(nextState);

    for (uint8_t i = 0; i < MAX_PORT_COUNT; i++) {
        PortInputMapping &port = nextState.ports[i];
        port.enabled = (i < nextState.activePortCount);

        if (!port.enabled) {
            nextState.statuses[i] = PORT_STATUS_DISABLED;
            continue;
        }

        const bool errorActive = inputs.active[port.errorInput];
        const bool chargingActive = inputs.active[port.chargingInput];

        if (errorActive) {
            nextState.statuses[i] = PORT_STATUS_ERROR;
        } else if (chargingActive) {
            nextState.statuses[i] = PORT_STATUS_CHARGING;
        } else {
            nextState.statuses[i] = PORT_STATUS_WAITING;
        }
    }

    portENTER_CRITICAL(&mapperStateMux);
    mapperState = nextState;
    portEXIT_CRITICAL(&mapperStateMux);
}

void status_mapper_get_snapshot(StatusMapperState &snapshot) {
    portENTER_CRITICAL(&mapperStateMux);
    snapshot = mapperState;
    portEXIT_CRITICAL(&mapperStateMux);
}

const char *status_mapper_status_to_string(PortStatus status) {
    switch (status) {
        case PORT_STATUS_WAITING:
            return "waiting";
        case PORT_STATUS_CHARGING:
            return "charging";
        case PORT_STATUS_ERROR:
            return "error";
        case PORT_STATUS_DISABLED:
            return "disabled";
        default:
            return "unknown";
    }
}
