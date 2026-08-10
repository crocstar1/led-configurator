#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ctype.h>
#include <inttypes.h>
#include <esp_system.h>
#include <esp_timer.h>
#include "firmware_version.h"
#include "gui_html.h"
#include "led_manager.h"
#include "matrix_state_storage.h"
#include "board_config.h"
#include "station_inputs.h"
#include "status_mapper.h"
#include "zone_model.h"
#include "network_config.h"
#include "auth_config.h"

WebServer server(80);
static MatrixStateV2 matrixStateCandidate;

bool otaUploadAuthorized = false;
bool otaUpdateSuccess = false;
bool otaRestartPending = false;
String otaUpdateError = "";
unsigned long otaRestartAt = 0;

static constexpr uint32_t INPUT_TASK_PERIOD_MS = 50;
static bool inputTaskStarted = false;
static uint64_t lastInputUpdateMs = 0;
static portMUX_TYPE inputRuntimeMux = portMUX_INITIALIZER_UNLOCKED;

static uint64_t monotonicMillis() {
    return (uint64_t)esp_timer_get_time() / 1000ULL;
}

static String uint64ToString(uint64_t value) {
    char buffer[21];
    snprintf(buffer, sizeof(buffer), "%" PRIu64, value);
    return String(buffer);
}

static void updatePortStatusFromInputs() {
    station_inputs_update();
    StationInputState inputs;
    station_inputs_get_snapshot(inputs);
    status_mapper_update(inputs);

    const uint64_t updatedAt = monotonicMillis();
    portENTER_CRITICAL(&inputRuntimeMux);
    lastInputUpdateMs = updatedAt;
    portEXIT_CRITICAL(&inputRuntimeMux);
}

static void inputTaskWorker(void *) {
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(INPUT_TASK_PERIOD_MS);

    while (true) {
        updatePortStatusFromInputs();
        vTaskDelayUntil(&lastWake, period);
    }
}

static void startInputTask() {
    const BaseType_t result = xTaskCreatePinnedToCore(
        inputTaskWorker,
        "Input_Task",
        2048,
        nullptr,
        2,
        nullptr,
        1
    );
    inputTaskStarted = result == pdPASS;
    if (!inputTaskStarted) {
        Serial.println("Input task start failed; using loop fallback");
    }
}

static uint64_t inputLastUpdateAgeMs() {
    uint64_t updatedAt;
    portENTER_CRITICAL(&inputRuntimeMux);
    updatedAt = lastInputUpdateMs;
    portEXIT_CRITICAL(&inputRuntimeMux);

    const uint64_t now = monotonicMillis();
    return updatedAt > 0 && now >= updatedAt ? now - updatedAt : 0;
}

static const char *resetReasonString() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON: return "power_on";
        case ESP_RST_EXT: return "external";
        case ESP_RST_SW: return "software";
        case ESP_RST_PANIC: return "panic";
        case ESP_RST_INT_WDT: return "interrupt_watchdog";
        case ESP_RST_TASK_WDT: return "task_watchdog";
        case ESP_RST_WDT: return "watchdog";
        case ESP_RST_DEEPSLEEP: return "deep_sleep";
        case ESP_RST_BROWNOUT: return "brownout";
        case ESP_RST_SDIO: return "sdio";
        case ESP_RST_UNKNOWN:
        default: return "unknown";
    }
}

bool authRequestAuthenticated() {
    return server.authenticate(auth_config_username(), auth_config_password());
}

bool requireHttpAuth() {
    if (authRequestAuthenticated()) return true;
    server.requestAuthentication();
    return false;
}

// Convert a HEX color string to RGB bytes.
void parseHexColor(String hex, uint8_t* targetArray) {
    if (hex.startsWith("#")) hex = hex.substring(1);
    if (hex.length() == 6) {
        targetArray[0] = strtol(hex.substring(0, 2).c_str(), NULL, 16); // R
        targetArray[1] = strtol(hex.substring(2, 4).c_str(), NULL, 16); // G
        targetArray[2] = strtol(hex.substring(4, 6).c_str(), NULL, 16); // B
    }
}

String rgbToHex(const uint8_t *rgb) {
    char buf[8];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", rgb[0], rgb[1], rgb[2]);
    return String(buf);
}

bool isValidHexColor(const String &value) {
    if (value.length() != 7 || value[0] != '#') return false;
    for (int i = 1; i < value.length(); i++) {
        if (!isxdigit((unsigned char)value[i])) return false;
    }
    return true;
}

String jsonEscape(String value) {
    value.replace("\\", "\\\\");
    value.replace("\"", "\\\"");
    value.replace("\n", "\\n");
    value.replace("\r", "\\r");
    return value;
}

String extractJsonStringField(const String &json, const char *fieldName) {
    String pattern = "\"";
    pattern += fieldName;
    pattern += "\"";

    int fieldPos = json.indexOf(pattern);
    if (fieldPos < 0) return "";

    int colonPos = json.indexOf(':', fieldPos + pattern.length());
    if (colonPos < 0) return "";

    int quoteStart = colonPos + 1;
    while (quoteStart < json.length() && isspace((unsigned char)json[quoteStart])) quoteStart++;
    if (quoteStart >= json.length() || json[quoteStart] != '"') return "";

    int quoteEnd = json.indexOf('"', quoteStart + 1);
    if (quoteEnd < 0) return "";

    return json.substring(quoteStart + 1, quoteEnd);
}

bool jsonHasField(const String &json, const char *fieldName) {
    String pattern = "\"";
    pattern += fieldName;
    pattern += "\"";
    return json.indexOf(pattern) >= 0;
}

bool parseStrictInt(const String &value, int &result) {
    String clean = value;
    clean.trim();
    if (clean.length() == 0) return false;

    int position = 0;
    bool negative = false;
    if (clean[position] == '-') {
        negative = true;
        position++;
    }
    if (position >= clean.length()) return false;

    long parsed = 0;
    for (; position < clean.length(); position++) {
        if (!isdigit((unsigned char)clean[position])) return false;
        const int digit = clean[position] - '0';
        if (parsed > (2147483647L - digit) / 10L) return false;
        parsed = (parsed * 10) + digit;
    }

    result = negative ? -(int)parsed : (int)parsed;
    return true;
}

bool extractJsonIntField(const String &json, const char *fieldName, int &result, bool allowBoolean = false) {
    String pattern = "\"";
    pattern += fieldName;
    pattern += "\"";

    int fieldPos = json.indexOf(pattern);
    if (fieldPos < 0) return false;

    int colonPos = json.indexOf(':', fieldPos + pattern.length());
    if (colonPos < 0) return false;

    int valueStart = colonPos + 1;
    while (valueStart < json.length() && isspace((unsigned char)json[valueStart])) valueStart++;

    if (allowBoolean) {
        const bool trueValue = json.substring(valueStart, valueStart + 4) == "true";
        const bool falseValue = json.substring(valueStart, valueStart + 5) == "false";
        if (trueValue || falseValue) {
            int trailing = valueStart + (trueValue ? 4 : 5);
            while (trailing < json.length() && isspace((unsigned char)json[trailing])) trailing++;
            if (trailing >= json.length() || (json[trailing] != ',' && json[trailing] != '}')) return false;
            result = trueValue ? 1 : 0;
            return true;
        }
    }

    int valueEnd = valueStart;
    if (valueEnd < json.length() && json[valueEnd] == '-') valueEnd++;
    while (valueEnd < json.length() && isdigit((unsigned char)json[valueEnd])) valueEnd++;

    if (valueEnd <= valueStart) return false;
    int trailing = valueEnd;
    while (trailing < json.length() && isspace((unsigned char)json[trailing])) trailing++;
    if (trailing < json.length() && json[trailing] != ',' && json[trailing] != '}') return false;
    return parseStrictInt(json.substring(valueStart, valueEnd), result);
}

bool extractJsonObjectField(const String &json, const char *fieldName, String &result) {
    String pattern = "\"";
    pattern += fieldName;
    pattern += "\"";

    int fieldPos = json.indexOf(pattern);
    if (fieldPos < 0) return false;

    int colonPos = json.indexOf(':', fieldPos + pattern.length());
    if (colonPos < 0) return false;

    int objectStart = colonPos + 1;
    while (objectStart < json.length() && isspace((unsigned char)json[objectStart])) objectStart++;
    if (objectStart >= json.length() || json[objectStart] != '{') return false;

    int depth = 0;
    for (int i = objectStart; i < json.length(); i++) {
        if (json[i] == '{') depth++;
        if (json[i] == '}') {
            depth--;
            if (depth == 0) {
                result = json.substring(objectStart, i + 1);
                return true;
            }
        }
    }

    return false;
}

bool parseFreeZoneMode(String mode, FreeZoneMode &result) {
    mode.toLowerCase();
    if (mode == "static") result = FREE_ZONE_STATIC;
    else if (mode == "custom") result = FREE_ZONE_CUSTOM;
    else if (mode == "rainbow") result = FREE_ZONE_RAINBOW;
    else return false;
    return true;
}

bool canonicalStatusName(const String &value, String &canonical) {
    if (value == "waiting" || value == "wait") canonical = "waiting";
    else if (value == "charging" || value == "charge") canonical = "charging";
    else if (value == "error" || value == "err") canonical = "error";
    else return false;
    return true;
}

bool isStorageError(const String &error) {
    return error.startsWith("NVS_");
}

bool storedLayerHasPixel(const MatrixStateStoredLayer &layer, size_t index) {
    return (layer.present[index / 8U] & (1U << (index % 8U))) != 0;
}

void appendStoredLayerJson(String &json, const MatrixStateV2 &state, size_t layerIndex) {
    json += "{";
    bool first = true;
    const size_t ledCount = (size_t)state.rows * state.cols;
    const MatrixStateStoredLayer &layer = state.layers[layerIndex];

    for (size_t logicalIndex = 0; logicalIndex < ledCount; logicalIndex++) {
        if (!storedLayerHasPixel(layer, logicalIndex)) continue;
        if (!first) json += ",";
        first = false;

        const size_t rgbOffset = logicalIndex * 3U;
        json += "\"";
        json += String((int)(logicalIndex % state.cols));
        json += "-";
        json += String((int)(logicalIndex / state.cols));
        json += "\":\"";
        json += rgbToHex(&layer.rgb[rgbOffset]);
        json += "\"";
    }
    json += "}";
}

void appendHardwareMapJson(String &json, const MatrixStateV2 &state) {
    json += ",\"hardware_map\":{";
    bool first = true;

    for (uint16_t y = 0; y < state.rows; y++) {
        for (uint16_t x = 0; x < state.cols; x++) {
            const size_t logicalIndex = (size_t)y * state.cols + x;
            const uint8_t zoneId = state.zoneMap[logicalIndex];
            if (zoneId == 0) continue;

            if (!first) json += ",";
            first = false;

            json += "\"";
            json += String(x);
            json += "-";
            json += String(y);
            json += "\":\"";
            json += String((int)zoneId);
            json += "\"";
        }
    }

    json += "}";
}

void appendStatusLayersJson(String &json, const MatrixStateV2 &state) {
    json += ",\"layers\":{\"wait\":";
    appendStoredLayerJson(json, state, MATRIX_STATE_LAYER_WAITING);
    json += ",\"charge\":";
    appendStoredLayerJson(json, state, MATRIX_STATE_LAYER_CHARGING);
    json += ",\"err\":";
    appendStoredLayerJson(json, state, MATRIX_STATE_LAYER_ERROR);
    json += "}";
}

FreeZoneMode currentFreeZoneMode(const MatrixStateV2 &state, uint8_t zoneId) {
    const int index = free_zone_index(zoneId);
    if (index >= 0) {
        return (FreeZoneMode)state.freeZones[index].mode;
    }

    const ZoneMetadata *metadata = zone_metadata_for(zoneId);
    return metadata ? metadata->defaultMode : FREE_ZONE_STATIC;
}

void appendFreeZoneConfigsJson(String &json, const MatrixStateV2 &state) {
    json += ",\"free_zones\":[";

    for (size_t i = 0; i < FREE_ZONE_COUNT; i++) {
        if (i > 0) json += ",";

        const MatrixStateStoredFreeZone &freeZone = state.freeZones[i];
        json += "{\"zoneId\":";
        json += String((int)freeZone.zoneId);
        json += ",\"enabled\":";
        json += (freeZone.enabled ? "true" : "false");
        json += ",\"mode\":\"";
        json += free_zone_mode_to_string((FreeZoneMode)freeZone.mode);
        json += "\",\"staticColor\":\"";
        json += rgbToHex(freeZone.staticColor);
        json += "\",\"brightness\":";
        json += String((int)freeZone.brightness);
        json += ",\"customLayer\":";
        appendStoredLayerJson(json, state, MATRIX_STATE_LAYER_LOGO + i);
        json += "}";
    }

    json += "]";
}

void appendZoneMetadataJson(String &json, const MatrixStateV2 &state) {
    const uint8_t activePortCount = state.activePortCount;

    json += ",\"zones\":[";

    for (size_t i = 0; i < ZONE_METADATA_COUNT; i++) {
        if (i > 0) json += ",";

        const ZoneMetadata &zone = ZONE_METADATA[i];
        bool enabled = zone.enabled;
        bool reserved = zone.reserved;

        if (zone.type == ZONE_TYPE_PORT) {
            const bool activePort = zone.linkedPort >= 0 && zone.linkedPort < activePortCount;
            enabled = activePort;
            reserved = !activePort;
        }

        json += "{\"id\":";
        json += String((int)zone.id);
        json += ",\"name\":\"";
        json += zone.name;
        json += "\",\"type\":\"";
        json += zone_type_to_string(zone.type);
        json += "\",\"enabled\":";
        json += (enabled ? "true" : "false");
        json += ",\"reserved\":";
        json += (reserved ? "true" : "false");

        if (zone.type == ZONE_TYPE_PORT) {
            json += ",\"linked_port\":";
            json += String((int)zone.linkedPort + 1);
        } else {
            json += ",\"linked_port\":null";
        }

        if (zone.type == ZONE_TYPE_FREE) {
            json += ",\"mode\":\"";
            json += free_zone_mode_to_string(currentFreeZoneMode(state, zone.id));
            json += "\"";
        } else {
            json += ",\"mode\":null";
        }

        json += "}";
    }

    json += "]";
}

bool activeLedCountForState(const MatrixStateV2 &state, size_t &ledCount) {
    if (state.rows == 0 || state.cols == 0 || state.rows > MAX_LED_COUNT / state.cols) {
        return false;
    }
    ledCount = (size_t)state.rows * state.cols;
    return ledCount > 0 && ledCount <= MAX_LED_COUNT;
}

bool zoneMapHasRequiredActivePortZones(const MatrixStateV2 &state) {
    size_t ledCount = 0;
    if (!activeLedCountForState(state, ledCount) ||
        state.activePortCount < 1 || state.activePortCount > MAX_PORT_COUNT) {
        return false;
    }

    for (uint8_t portIndex = 0; portIndex < state.activePortCount; portIndex++) {
        const uint8_t requiredZoneId = (uint8_t)(ZONE_ID_PORT1 + portIndex);
        bool found = false;
        for (size_t logicalIndex = 0; logicalIndex < ledCount; logicalIndex++) {
            if (state.zoneMap[logicalIndex] == requiredZoneId) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

void skipJsonWhitespace(const String &value, int &position) {
    while (position < value.length() && isspace((unsigned char)value[position])) position++;
}

bool parseMatrixCoordinate(
    const String &coordinate,
    const MatrixStateV2 &state,
    int &x,
    int &y) {
    const int dash = coordinate.indexOf('-');
    if (dash <= 0 || dash != coordinate.lastIndexOf('-') || dash >= coordinate.length() - 1) return false;
    return parseStrictInt(coordinate.substring(0, dash), x) &&
        parseStrictInt(coordinate.substring(dash + 1), y) &&
        x >= 0 && x < state.cols && y >= 0 && y < state.rows;
}

bool parseZoneMapPayload(const String &body, MatrixStateV2 &state) {
    size_t ledCount = 0;
    if (!activeLedCountForState(state, ledCount) ||
        state.activePortCount < 1 || state.activePortCount > MAX_PORT_COUNT ||
        body.length() < 2 ||
        body.length() > (2 + (ledCount * 24)) || body[0] != '{' ||
        body[body.length() - 1] != '}') {
        return false;
    }

    memset(state.zoneMap, ZONE_ID_OFF, sizeof(state.zoneMap));
    uint8_t seen[MATRIX_STATE_LAYER_MASK_BYTES] = {};
    size_t entryCount = 0;
    int position = 1;
    skipJsonWhitespace(body, position);
    if (position == body.length() - 1) return true;

    while (position < body.length() - 1) {
        if (body[position] != '"') return false;
        const int keyEnd = body.indexOf('"', position + 1);
        if (keyEnd < 0) return false;
        const String coordinate = body.substring(position + 1, keyEnd);
        position = keyEnd + 1;

        skipJsonWhitespace(body, position);
        if (position >= body.length() || body[position] != ':') return false;
        position++;
        skipJsonWhitespace(body, position);

        bool quoted = false;
        if (position < body.length() && body[position] == '"') {
            quoted = true;
            position++;
        }
        const int valueStart = position;
        if (position < body.length() && body[position] == '-') position++;
        while (position < body.length() && isdigit((unsigned char)body[position])) position++;
        if (position == valueStart || (body[valueStart] == '-' && position == valueStart + 1)) return false;
        const String zoneValue = body.substring(valueStart, position);
        if (quoted) {
            if (position >= body.length() || body[position] != '"') return false;
            position++;
        }

        int x = -1;
        int y = -1;
        int zoneId = -1;
        const int highestActivePortZone = ZONE_ID_PORT1 + state.activePortCount - 1;
        if (!parseMatrixCoordinate(coordinate, state, x, y) || !parseStrictInt(zoneValue, zoneId) ||
            zoneId < ZONE_ID_OFF || zoneId > ZONE_ID_MAX || !zone_id_is_valid((uint8_t)zoneId) ||
            (zone_is_port((uint8_t)zoneId) && zoneId > highestActivePortZone)) {
            return false;
        }
        const size_t logicalIndex = (size_t)y * state.cols + x;
        const uint8_t bit = (uint8_t)(1U << (logicalIndex % 8U));
        if ((seen[logicalIndex / 8U] & bit) != 0 || entryCount >= ledCount) return false;
        seen[logicalIndex / 8U] |= bit;
        state.zoneMap[logicalIndex] = (uint8_t)zoneId;
        entryCount++;

        skipJsonWhitespace(body, position);
        if (position == body.length() - 1) return true;
        if (body[position] != ',') return false;
        position++;
        skipJsonWhitespace(body, position);
        if (position >= body.length() - 1) return false;
    }

    return false;
}

bool parseStoredLayerPayload(
    const String &payload,
    const MatrixStateV2 &state,
    MatrixStateStoredLayer &layer,
    uint8_t firstColor[3],
    bool &hasFirstColor) {
    size_t ledCount = 0;
    if (!activeLedCountForState(state, ledCount)) return false;

    String body = payload;
    body.trim();
    if (body.length() < 2 || body.length() > (2 + (ledCount * 32)) ||
        body[0] != '{' || body[body.length() - 1] != '}') {
        return false;
    }

    memset(&layer, 0, sizeof(layer));
    hasFirstColor = false;
    size_t entryCount = 0;
    int position = 1;
    skipJsonWhitespace(body, position);
    if (position == body.length() - 1) return true;

    while (position < body.length() - 1) {
        if (body[position] != '"') return false;
        const int keyEnd = body.indexOf('"', position + 1);
        if (keyEnd < 0) return false;
        const String coordinate = body.substring(position + 1, keyEnd);
        position = keyEnd + 1;

        skipJsonWhitespace(body, position);
        if (position >= body.length() || body[position] != ':') return false;
        position++;
        skipJsonWhitespace(body, position);
        if (position >= body.length() || body[position] != '"') return false;
        const int colorEnd = body.indexOf('"', position + 1);
        if (colorEnd < 0) return false;
        const String colorText = body.substring(position + 1, colorEnd);
        if (!isValidHexColor(colorText)) return false;
        position = colorEnd + 1;

        int x = -1;
        int y = -1;
        if (!parseMatrixCoordinate(coordinate, state, x, y)) return false;
        const size_t logicalIndex = (size_t)y * state.cols + x;
        const uint8_t bit = (uint8_t)(1U << (logicalIndex % 8U));
        if ((layer.present[logicalIndex / 8U] & bit) != 0 || entryCount >= ledCount) return false;

        uint8_t parsedColor[3] = {};
        parseHexColor(colorText, parsedColor);
        layer.present[logicalIndex / 8U] |= bit;
        memcpy(&layer.rgb[logicalIndex * 3U], parsedColor, sizeof(parsedColor));
        if (!hasFirstColor) {
            memcpy(firstColor, parsedColor, sizeof(parsedColor));
            hasFirstColor = true;
        }
        entryCount++;

        skipJsonWhitespace(body, position);
        if (position == body.length() - 1) return true;
        if (body[position] != ',') return false;
        position++;
        skipJsonWhitespace(body, position);
        if (position >= body.length() - 1) return false;
    }
    return false;
}

const char *matrixSaveResultName(MatrixStateSaveResult result) {
    switch (result) {
        case MatrixStateSaveResult::SAVED: return "SAVED";
        case MatrixStateSaveResult::INVALID_STATE: return "INVALID_STATE";
        case MatrixStateSaveResult::VERIFY_FAILED: return "VERIFY_FAILED";
        case MatrixStateSaveResult::STORAGE_ERROR:
        default: return "STORAGE_ERROR";
    }
}

const char *matrixLoadResultName(MatrixStateLoadResult result) {
    switch (result) {
        case MatrixStateLoadResult::LOADED: return "LOADED";
        case MatrixStateLoadResult::NOT_FOUND: return "NOT_FOUND";
        case MatrixStateLoadResult::INCOMPATIBLE: return "INCOMPATIBLE";
        case MatrixStateLoadResult::CORRUPT: return "CORRUPT";
        case MatrixStateLoadResult::STORAGE_ERROR:
        default: return "STORAGE_ERROR";
    }
}

bool snapshotMatrixState() {
    if (led_get_matrix_state_snapshot(matrixStateCandidate)) return true;
    Serial.println("Matrix state snapshot failed");
    server.send(500, "text/plain", "STATE_UNAVAILABLE");
    return false;
}

bool saveAndApplyMatrixState(const char *saveError) {
    if (matrix_state_storage_validate(matrixStateCandidate) != MatrixStateValidationResult::VALID) {
        Serial.println("Matrix state candidate validation failed");
        server.send(500, "text/plain", saveError);
        return false;
    }

    const MatrixStateSaveResult saveResult = matrix_state_storage_save(matrixStateCandidate);
    if (saveResult != MatrixStateSaveResult::SAVED) {
        Serial.printf("Matrix state save failed: %s\n", matrixSaveResultName(saveResult));
        server.send(500, "text/plain", saveError);
        return false;
    }

    if (!led_apply_matrix_state_safe(matrixStateCandidate)) {
        Serial.println("Matrix state saved but runtime apply failed; saved state will load after reboot");
        server.send(500, "text/plain", "APPLY_FAILED_REBOOT_REQUIRED");
        return false;
    }
    return true;
}

void handleRoot() {
    if (!requireHttpAuth()) return;
    server.send_P(200, "text/html", PAGE_MAIN); 
}

void handleGetMatrixSize() {
    if (!requireHttpAuth()) return;
    if (!snapshotMatrixState()) return;
    String sizeStr = String(matrixStateCandidate.cols) + ":" + String(matrixStateCandidate.rows);
    server.send(200, "text/plain", sizeStr);
}

void handleGetConfig() {
    if (!requireHttpAuth()) return;
    if (!snapshotMatrixState()) return;

    size_t ledCount = 0;
    if (!activeLedCountForState(matrixStateCandidate, ledCount)) {
        server.send(500, "text/plain", "STATE_INVALID");
        return;
    }

    String json;
    json.reserve(7600);
    json += "{\"topo\":";
    json += String((int)matrixStateCandidate.topology);
    json += ",\"b_ports\":";
    json += String((int)matrixStateCandidate.portBrightness);
    json += ",\"c_wait\":\"";
    json += rgbToHex(matrixStateCandidate.waitingColor);
    json += "\",\"c_charge\":\"";
    json += rgbToHex(matrixStateCandidate.chargingColor);
    json += "\",\"c_error\":\"";
    json += rgbToHex(matrixStateCandidate.errorColor);
    json += "\",\"activePortCount\":";
    json += String((int)matrixStateCandidate.activePortCount);
    json += ",\"matrix\":{\"cols\":";
    json += String((int)matrixStateCandidate.cols);
    json += ",\"rows\":";
    json += String((int)matrixStateCandidate.rows);
    json += ",\"total\":";
    json += String((int)ledCount);
    json += ",\"topology\":";
    json += String((int)matrixStateCandidate.topology);
    json += "}";
    appendStatusLayersJson(json, matrixStateCandidate);
    appendHardwareMapJson(json, matrixStateCandidate);
    appendZoneMetadataJson(json, matrixStateCandidate);
    appendFreeZoneConfigsJson(json, matrixStateCandidate);
    json += "}";

    server.send(200, "application/json", json);
}

void handleSaveZones() {
    if (!requireHttpAuth()) return;

    String body = server.arg("plain");
    body.trim();
    if (!snapshotMatrixState()) return;
    if (!parseZoneMapPayload(body, matrixStateCandidate)) {
        server.send(400, "text/plain", "BAD_JSON");
        return;
    }

    if (!zoneMapHasRequiredActivePortZones(matrixStateCandidate)) {
        server.send(400, "text/plain", "ACTIVE_PORT_ZONE_EMPTY");
        return;
    }

    if (!saveAndApplyMatrixState("SAVE_FAILED")) return;
    server.send(200, "text/plain", "OK");
}

void handleSaveTopology() {
    if (!requireHttpAuth()) return;

    int topology = -1;
    if (server.hasArg("topology")) {
        if (!parseStrictInt(server.arg("topology"), topology)) topology = -1;
    } else {
        const String body = server.arg("plain");
        if (!extractJsonIntField(body, "topology", topology)) topology = -1;
    }

    if (topology < 0 || topology > 3) {
        server.send(400, "text/plain", "BAD_TOPOLOGY");
        return;
    }

    if (!snapshotMatrixState()) return;
    matrixStateCandidate.topology = (uint8_t)topology;
    if (!saveAndApplyMatrixState("SAVE_FAILED")) return;
    server.send(200, "text/plain", "OK");
}

void handleSetBright() {
    if (!requireHttpAuth()) return;

    if (!server.hasArg("type") || !server.hasArg("val")) {
        server.send(400, "text/plain", "MISSING_ARG");
        return;
    }

    const String type = server.arg("type");
    int parsedValue = 0;
    if (!parseStrictInt(server.arg("val"), parsedValue) || parsedValue < 1 || parsedValue > 255) {
        server.send(400, "text/plain", "BAD_BRIGHTNESS");
        return;
    }

    if (!snapshotMatrixState()) return;
    if (type == "ports") {
        matrixStateCandidate.portBrightness = (uint8_t)parsedValue;
    } else {
        server.send(400, "text/plain", "BAD_TYPE");
        return;
    }

    if (!saveAndApplyMatrixState("SAVE_FAILED")) return;
    server.send(200, "text/plain", "OK");
}

void handleSaveStatusColors() {
    if (!requireHttpAuth()) return;

    String body = server.arg("plain");
    body.trim();
    if (!snapshotMatrixState()) return;
    size_t ledCount = 0;
    if (!activeLedCountForState(matrixStateCandidate, ledCount)) {
        server.send(500, "text/plain", "STATE_INVALID");
        return;
    }
    if (!body.startsWith("{") || !body.endsWith("}") ||
        body.length() > (2 + (ledCount * 32) + 128)) {
        server.send(400, "text/plain", "BAD_JSON");
        return;
    }

    String status;
    if (!canonicalStatusName(extractJsonStringField(body, "status"), status)) {
        server.send(400, "text/plain", "BAD_STATUS");
        return;
    }

    String colorsJson;
    if (!extractJsonObjectField(body, "colors", colorsJson)) {
        server.send(400, "text/plain", "BAD_LAYER");
        return;
    }

    size_t layerIndex = MATRIX_STATE_LAYER_WAITING;
    uint8_t *fallbackColor = matrixStateCandidate.waitingColor;
    if (status == "charging") {
        layerIndex = MATRIX_STATE_LAYER_CHARGING;
        fallbackColor = matrixStateCandidate.chargingColor;
    } else if (status == "error") {
        layerIndex = MATRIX_STATE_LAYER_ERROR;
        fallbackColor = matrixStateCandidate.errorColor;
    }

    uint8_t firstColor[3] = {};
    bool hasFirstColor = false;
    if (!parseStoredLayerPayload(
            colorsJson,
            matrixStateCandidate,
            matrixStateCandidate.layers[layerIndex],
            firstColor,
            hasFirstColor)) {
        server.send(400, "text/plain", "BAD_LAYER");
        return;
    }
    if (hasFirstColor) memcpy(fallbackColor, firstColor, sizeof(firstColor));

    if (!saveAndApplyMatrixState("SAVE_LAYER_FAILED")) return;
    server.send(200, "text/plain", "OK");
}

void handleSaveFreeZone() {
    if (!requireHttpAuth()) return;

    String body = server.arg("plain");
    body.trim();
    if (!snapshotMatrixState()) return;
    size_t ledCount = 0;
    if (!activeLedCountForState(matrixStateCandidate, ledCount)) {
        server.send(500, "text/plain", "STATE_INVALID");
        return;
    }
    if (!body.startsWith("{") || !body.endsWith("}") ||
        body.length() > (2 + (ledCount * 32) + 512)) {
        server.send(400, "text/plain", "BAD_JSON");
        return;
    }

    int zoneIdValue = -1;
    if (!extractJsonIntField(body, "zoneId", zoneIdValue) ||
        zoneIdValue < 0 || zoneIdValue > 255 || !zone_is_free((uint8_t)zoneIdValue)) {
        server.send(400, "text/plain", "BAD_ZONE");
        return;
    }

    const int zoneIndex = free_zone_index((uint8_t)zoneIdValue);
    if (zoneIndex < 0) {
        server.send(400, "text/plain", "BAD_ZONE");
        return;
    }

    MatrixStateStoredFreeZone &nextZone = matrixStateCandidate.freeZones[zoneIndex];
    bool hasConfigPayload = false;

    if (jsonHasField(body, "enabled")) {
        int enabled = -1;
        if (!extractJsonIntField(body, "enabled", enabled, true) || (enabled != 0 && enabled != 1)) {
            server.send(400, "text/plain", "BAD_ENABLED");
            return;
        }
        nextZone.enabled = (uint8_t)enabled;
        hasConfigPayload = true;
    }

    if (jsonHasField(body, "mode")) {
        FreeZoneMode parsedMode = FREE_ZONE_STATIC;
        if (!parseFreeZoneMode(extractJsonStringField(body, "mode"), parsedMode)) {
            server.send(400, "text/plain", "BAD_MODE");
            return;
        }
        nextZone.mode = (uint8_t)parsedMode;
        hasConfigPayload = true;
    }

    if (jsonHasField(body, "brightness")) {
        int brightness = 0;
        if (!extractJsonIntField(body, "brightness", brightness) || brightness < 1 || brightness > 255) {
            server.send(400, "text/plain", "BAD_BRIGHTNESS");
            return;
        }
        nextZone.brightness = (uint8_t)brightness;
        hasConfigPayload = true;
    }

    if (jsonHasField(body, "staticColor")) {
        const String staticColor = extractJsonStringField(body, "staticColor");
        if (!isValidHexColor(staticColor)) {
            server.send(400, "text/plain", "BAD_COLOR");
            return;
        }
        parseHexColor(staticColor, nextZone.staticColor);
        hasConfigPayload = true;
    }

    const bool hasCustomLayer = jsonHasField(body, "customLayer");
    if (hasCustomLayer) {
        String customLayer;
        uint8_t ignoredFirstColor[3] = {};
        bool ignoredHasFirstColor = false;
        if (!extractJsonObjectField(body, "customLayer", customLayer) ||
            !parseStoredLayerPayload(
                customLayer,
                matrixStateCandidate,
                matrixStateCandidate.layers[MATRIX_STATE_LAYER_LOGO + zoneIndex],
                ignoredFirstColor,
                ignoredHasFirstColor)) {
            server.send(400, "text/plain", "BAD_LAYER");
            return;
        }

        const MatrixStateStoredLayer &newLayer =
            matrixStateCandidate.layers[MATRIX_STATE_LAYER_LOGO + zoneIndex];
        for (size_t logicalIndex = 0; logicalIndex < ledCount; logicalIndex++) {
            if (storedLayerHasPixel(newLayer, logicalIndex) &&
                matrixStateCandidate.zoneMap[logicalIndex] != nextZone.zoneId) {
                server.send(400, "text/plain", "BAD_LAYER");
                return;
            }
        }
    }

    if (!hasConfigPayload && !hasCustomLayer) {
        server.send(400, "text/plain", "NO_CHANGES");
        return;
    }

    const char *saveError = hasConfigPayload ? "SAVE_CONFIG_FAILED" : "SAVE_LAYER_FAILED";
    if (!saveAndApplyMatrixState(saveError)) return;
    server.send(200, "text/plain", "OK");
}

void handleNetworkStatus() {
    if (!requireHttpAuth()) return;

    const NetworkConfig &net = network_config_get();
    String json;
    json.reserve(940);
    json += "{\"mode\":\"";
    json += network_runtime_mode_string();
    json += "\",\"status\":\"";
    json += network_connection_status_string();
    json += "\",\"configured\":";
    json += network_has_saved_ssid() ? "true" : "false";
    json += ",\"connected\":";
    json += network_sta_connected() ? "true" : "false";
    json += ",\"ssid\":\"";
    json += jsonEscape(net.ssid);
    json += "\",\"passwordConfigured\":";
    json += net.password.length() > 0 ? "true" : "false";
    json += ",\"connectedSsid\":\"";
    json += jsonEscape(network_connected_ssid());
    json += "\",\"ip\":\"";
    json += network_local_ip_string();
    json += "\",\"staMac\":\"";
    json += jsonEscape(network_sta_mac_string());
    json += "\",\"rssi\":";
    json += String((int)network_rssi());
    json += ",\"dhcp\":";
    json += net.dhcp ? "true" : "false";
    json += ",\"staticIp\":\"";
    json += network_ip_to_string(net.staticIp);
    json += "\",\"gateway\":\"";
    json += network_ip_to_string(net.gateway);
    json += "\",\"subnet\":\"";
    json += network_ip_to_string(net.subnet);
    json += "\",\"dns\":\"";
    json += network_ip_to_string(net.dns);
    json += "\",\"defaultApPassword\":";
    json += network_uses_default_ap_password() ? "true" : "false";
    json += ",\"apSsid\":\"";
    json += jsonEscape(net.apSsid);
    json += "\",\"apIp\":\"";
    json += network_ap_ip_string();
    json += "\",\"apClients\":";
    json += String(network_ap_client_count());
    json += "}";

    server.send(200, "application/json", json);
}

void handleScanNetworks() {
    if (!requireHttpAuth()) return;

    if (!network_scan_is_active()) {
        if (!network_scan_start()) {
            server.send(500, "application/json", "{\"status\":\"failed\"}");
            return;
        }
        server.sendHeader("Retry-After", "1");
        server.send(202, "application/json", "{\"status\":\"scanning\"}");
        return;
    }

    const int count = network_scan_complete();
    if (count == WIFI_SCAN_RUNNING) {
        server.sendHeader("Retry-After", "1");
        server.send(202, "application/json", "{\"status\":\"scanning\"}");
        return;
    }
    if (count == WIFI_SCAN_FAILED) {
        network_scan_finish();
        server.send(500, "application/json", "{\"status\":\"failed\"}");
        return;
    }

    String json = "[";
    if (count > 0) {
        for (int i = 0; i < count; i++) {
            if (i > 0) json += ",";
            json += "{\"ssid\":\"";
            json += jsonEscape(WiFi.SSID(i));
            json += "\",\"rssi\":";
            json += String(WiFi.RSSI(i));
            json += ",\"channel\":";
            json += String(WiFi.channel(i));
            json += ",\"secure\":";
            json += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "false" : "true";
            json += "}";
        }
    }
    json += "]";
    network_scan_finish();

    server.send(200, "application/json", json);
}

void handleSaveNetworkConfig() {
    if (!requireHttpAuth()) return;

    NetworkConfig next = network_config_get();

    const String previousSsid = next.ssid;
    if (server.hasArg("ssid")) next.ssid = server.arg("ssid");
    if (server.hasArg("password")) {
        const String submittedPassword = server.arg("password");
        if (submittedPassword.length() > 0 || next.ssid != previousSsid) {
            next.password = submittedPassword;
        }
    }
    if (server.hasArg("dhcp")) {
        const String value = server.arg("dhcp");
        next.dhcp = value == "1" || value == "true" || value == "on";
    }
    if (!next.dhcp && server.hasArg("static_ip")) {
        IPAddress ip;
        if (!network_parse_ip(server.arg("static_ip"), ip)) {
            server.send(400, "text/plain", "BAD_STATIC_IP");
            return;
        }
        next.staticIp = ip;
    }
    if (!next.dhcp && server.hasArg("gateway")) {
        IPAddress ip;
        if (!network_parse_ip(server.arg("gateway"), ip)) {
            server.send(400, "text/plain", "BAD_GATEWAY");
            return;
        }
        next.gateway = ip;
    }
    if (!next.dhcp && server.hasArg("subnet")) {
        IPAddress ip;
        if (!network_parse_ip(server.arg("subnet"), ip)) {
            server.send(400, "text/plain", "BAD_SUBNET");
            return;
        }
        next.subnet = ip;
    }
    if (!next.dhcp && server.hasArg("dns")) {
        IPAddress ip;
        if (!network_parse_ip(server.arg("dns"), ip)) {
            server.send(400, "text/plain", "BAD_DNS");
            return;
        }
        next.dns = ip;
    }
    if (server.hasArg("ap_ssid")) next.apSsid = server.arg("ap_ssid");
    if (server.hasArg("ap_password") && server.arg("ap_password").length() > 0) {
        next.apPassword = server.arg("ap_password");
    }

    String error;
    if (!network_config_save(next, error)) {
        server.send(isStorageError(error) ? 500 : 400, "text/plain", error);
        return;
    }

    server.send(200, "text/plain", "OK");
}

void handleNetworkReconnect() {
    if (!requireHttpAuth()) return;
    network_schedule_apply(1500);
    server.send(200, "text/plain", "RECONNECT_SCHEDULED");
}

void handleAuthStatus() {
    if (!requireHttpAuth()) return;

    String json = "{\"username\":\"";
    json += jsonEscape(auth_config_username_string());
    json += "\",\"defaultCredentials\":";
    json += auth_config_uses_default_credentials() ? "true" : "false";
    json += "}";
    server.send(200, "application/json", json);
}

void handleSaveAuth() {
    if (!requireHttpAuth()) return;

    String username = server.arg("username");
    String password = server.arg("password");
    String confirm = server.hasArg("confirm") ? server.arg("confirm") : server.arg("password_confirm");

    username.trim();
    if (username.length() == 0) {
        server.send(400, "text/plain", "USERNAME_REQUIRED");
        return;
    }
    if (password.length() == 0) {
        server.send(400, "text/plain", "PASSWORD_REQUIRED");
        return;
    }
    if (password != confirm) {
        server.send(400, "text/plain", "PASSWORD_CONFIRM_MISMATCH");
        return;
    }

    String error;
    if (!auth_config_save_credentials(username, password, error)) {
        server.send(isStorageError(error) ? 500 : 400, "text/plain", error);
        return;
    }

    server.send(200, "text/plain", "OK");
}

void handleDiagnostics() {
    if (!requireHttpAuth()) return;

    StationInputState inputs;
    StatusMapperState status;
    LedRuntimeState ledRuntime;
    station_inputs_get_snapshot(inputs);
    status_mapper_get_snapshot(status);
    led_get_runtime_state(ledRuntime);

    String json;
    json.reserve(2100);
    json += "{\"firmware\":{\"version\":\"";
    json += FIRMWARE_VERSION;
    json += "\",\"build\":\"";
    json += FIRMWARE_BUILD_ID;
    json += "\",\"environment\":\"";
    json += FIRMWARE_BUILD_ENVIRONMENT;
    json += "\"},\"system\":{\"uptimeSec\":";
    json += uint64ToString((uint64_t)(esp_timer_get_time() / 1000000LL));
    json += ",\"freeHeap\":";
    json += String((unsigned int)ESP.getFreeHeap());
    json += ",\"minFreeHeap\":";
    json += String((unsigned int)ESP.getMinFreeHeap());
    json += ",\"resetReason\":\"";
    json += resetReasonString();
    json += "\"},\"inputRuntime\":{\"taskStarted\":";
    json += inputTaskStarted ? "true" : "false";
    json += ",\"loopFallback\":";
    json += inputTaskStarted ? "false" : "true";
    json += ",\"lastUpdateAgeMs\":";
    json += uint64ToString(inputLastUpdateAgeMs());
    json += "},\"networkRuntime\":{\"apStartState\":\"";
    json += network_ap_start_state_string();
    json += "\"},\"activePortCount\":";
    json += String((int)status.activePortCount);
    json += ",\"runtimeMode\":\"runtime\"";
    json += ",\"ledRuntime\":{\"mutexReady\":";
    json += ledRuntime.mutexReady ? "true" : "false";
    json += ",\"taskStarted\":";
    json += ledRuntime.taskStarted ? "true" : "false";
    json += ",\"loopFallback\":";
    json += ledRuntime.loopFallback ? "true" : "false";
    json += ",\"lastRefreshAgeMs\":";
    json += uint64ToString(ledRuntime.lastRefreshAgeMs);
    json += "}";
    json += ",\"primaryLedOutput\":{\"name\":\"LED1\",\"gpio\":";
    json += String((int)PRIMARY_LED_PIN);
    json += ",\"index\":";
    json += String((int)PRIMARY_LED_OUTPUT_INDEX + 1);
    json += "},\"ledOutputs\":[";

    for (uint8_t i = 0; i < LED_OUTPUT_COUNT; i++) {
        if (i > 0) json += ",";
        json += "{\"name\":\"LED";
        json += String((int)i + 1);
        json += "\",\"gpio\":";
        json += String((int)LED_OUTPUT_PINS[i]);
        json += ",\"state\":\"";
        json += ((i == PRIMARY_LED_OUTPUT_INDEX) ? "primary" : "reserved");
        json += "\"}";
    }

    json += "],\"inputs\":[";
    for (uint8_t i = 0; i < BOARD_INPUT_COUNT; i++) {
        if (i > 0) json += ",";
        json += "{\"name\":\"Data";
        json += String((int)i + 1);
        json += "\",\"gpio\":";
        json += String((int)BOARD_INPUT_PINS[i]);
        json += ",\"raw\":\"";
        json += ((inputs.rawLevel[i] == LOW) ? "LOW" : "HIGH");
        json += "\",\"active\":";
        json += (inputs.active[i] ? "true" : "false");
        json += "}";
    }

    json += "],\"ports\":[";
    for (uint8_t i = 0; i < MAX_PORT_COUNT; i++) {
        if (i > 0) json += ",";
        const PortInputMapping &port = status.ports[i];
        json += "{\"port\":";
        json += String((int)i + 1);
        json += ",\"enabled\":";
        json += (port.enabled ? "true" : "false");
        json += ",\"zoneId\":";
        json += String((int)port.zoneId);
        json += ",\"chargingInput\":\"Data";
        json += String((int)port.chargingInput + 1);
        json += "\",\"errorInput\":\"Data";
        json += String((int)port.errorInput + 1);
        json += "\",\"status\":\"";
        json += status_mapper_status_to_string(status.statuses[i]);
        json += "\"}";
    }
    json += "]}";

    server.send(200, "application/json", json);
}

void setup() {
    Serial.begin(115200);
    auth_config_setup();

    matrix_state_storage_set_factory_defaults(matrixStateCandidate);
    const MatrixStateLoadResult matrixLoadResult =
        matrix_state_storage_load(matrixStateCandidate);
    if (matrixLoadResult == MatrixStateLoadResult::LOADED) {
        Serial.println("Matrix state V2 loaded");
    } else {
        Serial.printf("Matrix state V2 %s; using factory defaults without saving\n",
            matrixLoadResultName(matrixLoadResult));
    }

    station_inputs_setup();
    if (!status_mapper_setup(matrixStateCandidate.activePortCount)) {
        Serial.println("Matrix active port count is invalid; startup stopped");
        return;
    }
    updatePortStatusFromInputs();
    led_setup(matrixStateCandidate);
    startInputTask();
    network_setup();

    Serial.println("\n=== LED matrix controller network started ===");
    if (network_sta_connected()) {
        Serial.print("Configurator address: http://");
        Serial.println(network_local_ip_string());
    } else if (network_runtime_mode() == NETWORK_MODE_AP_FALLBACK) {
        Serial.print("Configurator address: http://");
        Serial.println(network_ap_ip_string());
    } else {
        Serial.println("Configurator address pending WiFi connection");
    }

    server.on("/", handleRoot);
    server.on("/get_size", handleGetMatrixSize);
    server.on("/get_config", handleGetConfig);
    server.on("/save_zones", HTTP_POST, handleSaveZones);
    server.on("/save_topology", HTTP_POST, handleSaveTopology);
    server.on("/set_bright", HTTP_POST, handleSetBright);
    server.on("/save_status_colors", HTTP_POST, handleSaveStatusColors);
    server.on("/save_free_zone", HTTP_POST, handleSaveFreeZone);
    server.on("/network_status", HTTP_GET, handleNetworkStatus);
    server.on("/scan_networks", HTTP_GET, handleScanNetworks);
    server.on("/save_network", HTTP_POST, handleSaveNetworkConfig);
    server.on("/network_reconnect", HTTP_POST, handleNetworkReconnect);
    server.on("/auth_status", HTTP_GET, handleAuthStatus);
    server.on("/save_auth", HTTP_POST, handleSaveAuth);
    server.on("/diagnostics", handleDiagnostics);

    server.on("/update", HTTP_POST, []() {
        if (!requireHttpAuth()) return;
        server.sendHeader("Connection", "close");
        if (otaUpdateSuccess && !Update.hasError()) {
            otaRestartPending = true;
            otaRestartAt = millis() + 1000;
            server.send(200, "text/plain", "OK");
        } else {
            server.send(500, "text/plain", otaUpdateError.length() ? otaUpdateError : "UPDATE_FAILED");
        }
    }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            otaUploadAuthorized = authRequestAuthenticated();
            otaUpdateSuccess = false;
            otaUpdateError = "";
            if (!otaUploadAuthorized) {
                otaUpdateError = "AUTH_REQUIRED";
                return;
            }
            if (upload.filename.length() == 0) {
                otaUpdateError = "NO_FILE";
                return;
            }
            String uploadName = upload.filename;
            uploadName.toLowerCase();
            if (!uploadName.endsWith(".bin")) {
                otaUpdateError = "BAD_FILE_TYPE";
                return;
            }
            Serial.printf("OTA start: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                otaUpdateError = "UPDATE_BEGIN_FAILED";
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (!otaUploadAuthorized || otaUpdateError.length()) return;
            if (upload.currentSize == 0) {
                return;
            }
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                otaUpdateError = "UPDATE_WRITE_FAILED";
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (!otaUploadAuthorized) return;
            if (otaUpdateError.length() == 0 && upload.totalSize == 0) {
                otaUpdateError = "EMPTY_FILE";
            }
            if (otaUpdateError.length() == 0 && Update.end(true)) {
                otaUpdateSuccess = true;
                Serial.printf("OTA complete: %u bytes\n", upload.totalSize);
            } else {
                if (otaUpdateError.length() == 0) otaUpdateError = "UPDATE_END_FAILED";
                Update.printError(Serial);
            }
            otaUploadAuthorized = false;
        } else if (upload.status == UPLOAD_FILE_ABORTED) {
            otaUpdateError = "UPLOAD_ABORTED";
            otaUploadAuthorized = false;
            Update.abort();
        }
    });
    server.begin();
}

void loop() {
    server.handleClient();
    network_loop();
    led_loop();
    if (!inputTaskStarted) {
        static unsigned long lastFallbackInputUpdate = 0;
        const unsigned long now = millis();
        if (now - lastFallbackInputUpdate >= INPUT_TASK_PERIOD_MS) {
            lastFallbackInputUpdate = now;
            updatePortStatusFromInputs();
        }
    }
    if (otaRestartPending && millis() >= otaRestartAt) {
        otaRestartPending = false;
        ESP.restart();
    }
}
