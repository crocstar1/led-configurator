#include "status_mapper.h"

static StatusMapperState mapperState = {};

void status_mapper_setup() {
    mapperState.activePortCount = USP1_DEFAULT_ACTIVE_PORT_COUNT;

    for (uint8_t i = 0; i < USP1_MAX_PORT_COUNT; i++) {
        mapperState.ports[i].chargingInput = i * 2;
        mapperState.ports[i].errorInput = (i * 2) + 1;
        mapperState.ports[i].zoneId = i + 1;
        mapperState.ports[i].enabled = (i < mapperState.activePortCount);
        mapperState.statuses[i] = mapperState.ports[i].enabled
            ? PORT_STATUS_WAITING
            : PORT_STATUS_DISABLED;
    }
}

void status_mapper_update(const Usp1InputState &inputs) {
    for (uint8_t i = 0; i < USP1_MAX_PORT_COUNT; i++) {
        PortInputMapping &port = mapperState.ports[i];
        port.enabled = (i < mapperState.activePortCount);

        if (!port.enabled) {
            mapperState.statuses[i] = PORT_STATUS_DISABLED;
            continue;
        }

        const bool errorActive = inputs.active[port.errorInput];
        const bool chargingActive = inputs.active[port.chargingInput];

        if (errorActive) {
            mapperState.statuses[i] = PORT_STATUS_ERROR;
        } else if (chargingActive) {
            mapperState.statuses[i] = PORT_STATUS_CHARGING;
        } else {
            mapperState.statuses[i] = PORT_STATUS_WAITING;
        }
    }
}

const StatusMapperState &status_mapper_get_state() {
    return mapperState;
}

void status_mapper_set_active_port_count(uint8_t activePortCount) {
    if (activePortCount > USP1_MAX_PORT_COUNT) {
        activePortCount = USP1_MAX_PORT_COUNT;
    }
    mapperState.activePortCount = activePortCount;

    for (uint8_t i = 0; i < USP1_MAX_PORT_COUNT; i++) {
        mapperState.ports[i].enabled = (i < mapperState.activePortCount);
        if (!mapperState.ports[i].enabled) {
            mapperState.statuses[i] = PORT_STATUS_DISABLED;
        }
    }
}

bool status_mapper_configure_port(uint8_t portIndex, uint8_t chargingInput, uint8_t errorInput, uint8_t zoneId) {
    if (portIndex >= USP1_MAX_PORT_COUNT ||
        chargingInput >= USP1_DATA_INPUT_COUNT ||
        errorInput >= USP1_DATA_INPUT_COUNT) {
        return false;
    }

    mapperState.ports[portIndex].chargingInput = chargingInput;
    mapperState.ports[portIndex].errorInput = errorInput;
    mapperState.ports[portIndex].zoneId = zoneId;
    mapperState.ports[portIndex].enabled = (portIndex < mapperState.activePortCount);
    return true;
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
