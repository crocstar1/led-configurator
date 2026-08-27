#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ctype.h>
#include <inttypes.h>
#include <esp_ota_ops.h>
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

extern "C" bool verifyRollbackLater() {
    return true;
}

WebServer server(80);
static MatrixStateV3 matrixStateCandidate;
static uint8_t previousZoneMap[MAX_TOTAL_LED_COUNT];

bool otaUploadAuthorized = false;
bool otaUpdateSuccess = false;
bool otaRestartPending = false;
String otaUpdateError = "";
unsigned long otaRestartAt = 0;

static bool controllerRestartPending = false;
static unsigned long controllerRestartAt = 0;
static bool otaRollbackVerificationPending = false;
static uint32_t otaRollbackVerificationStartedAt = 0;

static bool hardwareConfigRestartRequired = false;
static bool hardwareConfigRestartScheduled = false;
static bool matrixUiRequiresMultiOutput = false;
static unsigned long hardwareConfigRestartAt = 0;

static constexpr uint32_t INPUT_TASK_PERIOD_MS = 50;
static constexpr uint32_t HARDWARE_CONFIG_RESTART_DELAY_MS = 1000;
static constexpr uint32_t CONTROLLER_RESTART_DELAY_MS = 1000;
static constexpr uint32_t SERVICE_BUTTON_DEBOUNCE_MS = 40;
static constexpr uint32_t SERVICE_BUTTON_AP_HOLD_MS = 3000;
static constexpr uint32_t SERVICE_BUTTON_RESET_HOLD_MS = 10000;
static constexpr uint32_t OTA_ROLLBACK_VERIFY_DELAY_MS = 5000;
static constexpr uint8_t MATRIX_TOPOLOGY_MAX = 7;
// Worst current 500-pixel chunk is 9007 bytes (a full free-zone layer at 1x500/500x1).
static constexpr size_t GET_CONFIG_CHUNK_RESERVE = 12U * 1024U;
static constexpr size_t HARDWARE_CONFIG_BODY_LIMIT = 1024U;

struct MultiOutputHardwareConfig {
    uint8_t activePortCount;
    MatrixStateV3Output outputs[LED_OUTPUT_COUNT];
};

struct FactoryResetResult {
    bool matrixCleared;
    bool networkCleared;
    bool authCleared;
};

static bool inputTaskStarted = false;
static uint64_t lastInputUpdateMs = 0;
static portMUX_TYPE inputRuntimeMux = portMUX_INITIALIZER_UNLOCKED;
static bool serviceButtonRawPressed = false;
static bool serviceButtonStablePressed = false;
static bool serviceButtonArmed = false;
static bool serviceButtonApHandled = false;
static bool serviceButtonResetHandled = false;
static bool serviceButtonRestartPending = false;
static unsigned long serviceButtonRawChangedAt = 0;
static unsigned long serviceButtonHoldStartedAt = 0;

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

static bool serviceButtonIsPressed() {
    return digitalRead(BOARD_SERVICE_BUTTON_PIN) ==
        BOARD_SERVICE_BUTTON_ACTIVE_LEVEL;
}

static FactoryResetResult clearAllUserSettings() {
    const bool ledShutdownPrepared = led_prepare_for_restart();
    Serial.printf("Factory reset LED shutdown: %s\n",
        ledShutdownPrepared ? "success" : "failed");

    FactoryResetResult result = {
        matrix_state_storage_clear_saved_state(),
        network_config_clear_saved_settings(),
        auth_config_clear_saved_settings(),
    };
    Serial.printf("Factory reset matrix state: %s\n",
        result.matrixCleared ? "success" : "failed");
    Serial.printf("Factory reset net_cfg: %s\n",
        result.networkCleared ? "cleared" : "failed");
    Serial.printf("Factory reset auth_cfg: %s\n",
        result.authCleared ? "cleared" : "failed");
    return result;
}

static bool factoryResetSucceeded(const FactoryResetResult &result) {
    return result.matrixCleared && result.networkCleared && result.authCleared;
}

static void setupServiceButton() {
    pinMode(BOARD_SERVICE_BUTTON_PIN, INPUT_PULLUP);
    serviceButtonRawPressed = serviceButtonIsPressed();
    serviceButtonStablePressed = serviceButtonRawPressed;
    serviceButtonRawChangedAt = millis();
    // A low level already present during setup may come from the programmer.
    // Require one stable release before accepting a service-button hold.
    serviceButtonArmed = !serviceButtonStablePressed;
}

static void handleServiceButtonApHold() {
    serviceButtonApHandled = true;
    Serial.println("BOOT service button held for 3 seconds");
    const bool apStarted = network_force_ap_fallback();
    Serial.printf("BOOT forced AP: %s\n", apStarted ? "enabled" : "failed");
    Serial.print("BOOT AP SSID: ");
    Serial.println(network_config_get().apSsid);
    Serial.print("BOOT AP IP: ");
    Serial.println(network_ap_ip_string());
}

static void handleServiceButtonFactoryReset() {
    serviceButtonResetHandled = true;
    Serial.println("BOOT service button held for 10 seconds; factory reset started");

    const FactoryResetResult resetResult = clearAllUserSettings();
    if (factoryResetSucceeded(resetResult)) {
        serviceButtonRestartPending = true;
        Serial.println("Factory reset complete; release BOOT to restart ESP32");
        return;
    }

    const bool apStarted = network_force_ap_fallback();
    Serial.printf("Factory reset incomplete; forced AP remains %s. "
                  "Release BOOT and hold it again to retry.\n",
        apStarted ? "enabled" : "unavailable");
}

static void pollServiceButton() {
    const unsigned long now = millis();
    const bool rawPressed = serviceButtonIsPressed();
    if (rawPressed != serviceButtonRawPressed) {
        serviceButtonRawPressed = rawPressed;
        serviceButtonRawChangedAt = now;
    }

    if (serviceButtonRawPressed != serviceButtonStablePressed &&
        now - serviceButtonRawChangedAt >= SERVICE_BUTTON_DEBOUNCE_MS) {
        serviceButtonStablePressed = serviceButtonRawPressed;
        if (serviceButtonStablePressed) {
            if (serviceButtonArmed) {
                serviceButtonHoldStartedAt = serviceButtonRawChangedAt;
                serviceButtonApHandled = false;
                serviceButtonResetHandled = false;
                serviceButtonRestartPending = false;
            }
        } else {
            if (serviceButtonRestartPending) {
                Serial.println("BOOT released after factory reset; restarting ESP32");
                Serial.flush();
                ESP.restart();
            }
            serviceButtonArmed = true;
            serviceButtonHoldStartedAt = 0;
            serviceButtonApHandled = false;
            serviceButtonResetHandled = false;
            serviceButtonRestartPending = false;
        }
    }

    if (!serviceButtonArmed || !serviceButtonStablePressed ||
        !serviceButtonRawPressed) return;

    const unsigned long heldMs = now - serviceButtonHoldStartedAt;
    if (!serviceButtonApHandled && heldMs >= SERVICE_BUTTON_AP_HOLD_MS) {
        handleServiceButtonApHold();
    }
    if (!serviceButtonResetHandled && !controllerRestartPending &&
        heldMs >= SERVICE_BUTTON_RESET_HOLD_MS) {
        handleServiceButtonFactoryReset();
    }
}

bool authRequestAuthenticated() {
    if (!auth_config_enabled()) return true;
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

bool storedLayerHasPixel(const MatrixStateV3StoredLayer &layer, size_t index) {
    return (layer.present[index / 8U] & (1U << (index % 8U))) != 0;
}

void clearStoredLayerPixel(MatrixStateV3StoredLayer &layer, size_t index) {
    const uint8_t bit = (uint8_t)(1U << (index % 8U));
    layer.present[index / 8U] &= (uint8_t)~bit;
    const size_t rgbOffset = index * 3U;
    layer.rgb[rgbOffset] = 0;
    layer.rgb[rgbOffset + 1U] = 0;
    layer.rgb[rgbOffset + 2U] = 0;
}

void appendStoredLayerJson(
    String &json,
    const MatrixStateV3 &state,
    size_t layerIndex,
    size_t outputIndex,
    const MatrixStateV3OutputLayout &outputLayout) {
    json += "{";
    bool first = true;
    const MatrixStateV3Output &output = state.outputs[outputIndex];
    const MatrixStateV3StoredLayer &layer = state.layers[layerIndex];

    for (size_t localIndex = 0; localIndex < outputLayout.length; localIndex++) {
        const size_t globalIndex = outputLayout.offset + localIndex;
        if (!storedLayerHasPixel(layer, globalIndex)) continue;
        if (!first) json += ",";
        first = false;

        const size_t rgbOffset = globalIndex * 3U;
        json += "\"";
        json += String((int)(localIndex % output.cols));
        json += "-";
        json += String((int)(localIndex / output.cols));
        json += "\":\"";
        json += rgbToHex(&layer.rgb[rgbOffset]);
        json += "\"";
    }
    json += "}";
}

void appendHardwareMapJson(
    String &json,
    const MatrixStateV3 &state,
    size_t outputIndex,
    const MatrixStateV3OutputLayout &outputLayout) {
    json += ",\"hardware_map\":{";
    bool first = true;

    const MatrixStateV3Output &output = state.outputs[outputIndex];
    for (size_t localIndex = 0; localIndex < outputLayout.length; localIndex++) {
        const size_t globalIndex = outputLayout.offset + localIndex;
        const uint8_t zoneId = state.zoneMap[globalIndex];
        if (zoneId == 0) continue;

        if (!first) json += ",";
        first = false;

        json += "\"";
        json += String((int)(localIndex % output.cols));
        json += "-";
        json += String((int)(localIndex / output.cols));
        json += "\":\"";
        json += String((int)zoneId);
        json += "\"";
    }

    json += "}";
}

void sendAndClearConfigChunk(String &json) {
    server.sendContent(json);
    json.remove(0);
}

FreeZoneMode currentFreeZoneMode(
    const MatrixStateV3 &state,
    size_t outputIndex,
    uint8_t zoneId) {
    const int index = free_zone_index(zoneId);
    if (index >= 0) {
        return (FreeZoneMode)state.freeZones[outputIndex][index].mode;
    }

    const ZoneMetadata *metadata = zone_metadata_for(zoneId);
    return metadata ? metadata->defaultMode : FREE_ZONE_STATIC;
}

void appendFreeZoneConfigJson(
    String &json,
    const MatrixStateV3 &state,
    size_t outputIndex,
    const MatrixStateV3OutputLayout &outputLayout,
    size_t index) {
    const MatrixStateStoredFreeZone &freeZone =
        state.freeZones[outputIndex][index];
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
    appendStoredLayerJson(
        json,
        state,
        MATRIX_STATE_LAYER_LOGO + index,
        outputIndex,
        outputLayout);
    json += "}";
}

static bool restartAlreadyScheduled() {
    return controllerRestartPending || hardwareConfigRestartScheduled ||
        otaRestartPending || serviceButtonRestartPending;
}

static bool otaUploadInProgress() {
    return otaUploadAuthorized || Update.isRunning();
}

static bool rejectSaveForControllerRestart() {
    if (!controllerRestartPending) return false;
    server.send(409, "text/plain", "RESTART_PENDING");
    return true;
}

static void scheduleControllerRestart() {
    controllerRestartPending = true;
    controllerRestartAt = millis() + CONTROLLER_RESTART_DELAY_MS;
}

void handleControllerRestart() {
    if (!requireHttpAuth()) return;
    if (otaUploadInProgress()) {
        server.send(409, "text/plain", "OTA_IN_PROGRESS");
        return;
    }
    if (restartAlreadyScheduled()) {
        server.send(409, "text/plain", "RESTART_PENDING");
        return;
    }

    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", "RESTARTING");
    scheduleControllerRestart();
    Serial.println("Web restart requested; restarting ESP32 in 1 second");
}

void handleControllerFactoryReset() {
    if (!requireHttpAuth()) return;
    if (otaUploadInProgress()) {
        server.send(409, "text/plain", "OTA_IN_PROGRESS");
        return;
    }
    if (restartAlreadyScheduled()) {
        server.send(409, "text/plain", "RESTART_PENDING");
        return;
    }

    Serial.println("Web factory reset started");
    const FactoryResetResult resetResult = clearAllUserSettings();
    if (!factoryResetSucceeded(resetResult)) {
        Serial.println("Web factory reset incomplete; automatic restart cancelled");
        server.send(500, "text/plain", "RESET_FAILED");
        return;
    }

    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", "RESETTING");
    scheduleControllerRestart();
    Serial.println("Web factory reset complete; restarting ESP32 in 1 second");
}

void appendZoneMetadataJson(
    String &json,
    const MatrixStateV3 &state,
    size_t outputIndex) {
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
            json += free_zone_mode_to_string(
                currentFreeZoneMode(state, outputIndex, zone.id));
            json += "\"";
        } else {
            json += ",\"mode\":null";
        }

        json += "}";
    }

    json += "]";
}

bool activeLedCountForState(const MatrixStateV3 &state, size_t &ledCount) {
    const MatrixStateV3Output &output = state.outputs[0];
    if (output.enabled != 1 || output.rows == 0 || output.cols == 0 ||
        output.rows > MAX_LED_COUNT_PER_OUTPUT / output.cols) {
        return false;
    }
    ledCount = (size_t)output.rows * output.cols;
    return ledCount > 0 && ledCount <= MAX_LED_COUNT_PER_OUTPUT;
}

bool currentUiSupportsMatrixState(const MatrixStateV3 &state) {
    size_t led1Count = 0;
    if (!activeLedCountForState(state, led1Count)) return false;

    for (size_t outputIndex = 1; outputIndex < LED_OUTPUT_COUNT;
         outputIndex++) {
        const MatrixStateV3Output &output = state.outputs[outputIndex];
        if (output.enabled != 0 || output.rows != 0 || output.cols != 0) {
            return false;
        }
    }
    return true;
}

const char *statusLightModeApiName(StatusLightMode mode) {
    return mode == StatusLightMode::PULSE_MODE ? "pulse" : "static";
}

bool parseStatusLightModeApiName(
    const String &value,
    StatusLightMode &mode) {
    if (value == "static") mode = StatusLightMode::STATIC_MODE;
    else if (value == "pulse") mode = StatusLightMode::PULSE_MODE;
    else return false;
    return true;
}

void pruneFreeCustomLayersForZoneMapSegment(
    MatrixStateV3 &state,
    const MatrixStateV3OutputLayout &outputLayout) {
    for (size_t zoneIndex = 0; zoneIndex < FREE_ZONE_COUNT; zoneIndex++) {
        const uint8_t zoneId = FREE_ZONE_IDS[zoneIndex];
        MatrixStateV3StoredLayer &layer =
            state.layers[MATRIX_STATE_LAYER_LOGO + zoneIndex];

        for (size_t localIndex = 0; localIndex < outputLayout.length;
             localIndex++) {
            const size_t globalIndex = outputLayout.offset + localIndex;
            if (state.zoneMap[globalIndex] == zoneId) continue;

            clearStoredLayerPixel(layer, globalIndex);
        }
    }
}

void clearStatusPixelsForChangedZones(
    MatrixStateV3 &state,
    const uint8_t oldZoneMap[MAX_TOTAL_LED_COUNT],
    const MatrixStateV3OutputLayout &outputLayout) {
    for (size_t localIndex = 0; localIndex < outputLayout.length;
         localIndex++) {
        const size_t globalIndex = outputLayout.offset + localIndex;
        if (oldZoneMap[globalIndex] == state.zoneMap[globalIndex]) continue;
        for (size_t layerIndex = MATRIX_STATE_LAYER_WAITING;
             layerIndex <= MATRIX_STATE_LAYER_ERROR;
             layerIndex++) {
            clearStoredLayerPixel(state.layers[layerIndex], globalIndex);
        }
    }
}

bool statusLayerUsesOnlyActivePortZones(
    const MatrixStateV3 &state,
    const MatrixStateV3StoredLayer &layer,
    const MatrixStateV3OutputLayout &outputLayout) {
    for (size_t localIndex = 0; localIndex < outputLayout.length;
         localIndex++) {
        const size_t globalIndex = outputLayout.offset + localIndex;
        if (!storedLayerHasPixel(layer, globalIndex)) continue;
        const uint8_t zoneId = state.zoneMap[globalIndex];
        if (!zone_is_port(zoneId) ||
            zoneId - ZONE_ID_PORT1 >= state.activePortCount) {
            return false;
        }
    }
    return true;
}

void skipJsonWhitespace(const String &value, int &position) {
    while (position < value.length() && isspace((unsigned char)value[position])) position++;
}

const char *ledProtocolApiName(LedProtocol protocol) {
    switch (protocol) {
        case LedProtocol::WS2811_800K: return "ws2811_800k";
        default: return nullptr;
    }
}

const char *ledColorOrderApiName(LedColorOrder colorOrder) {
    switch (colorOrder) {
        case LedColorOrder::RGB: return "RGB";
        case LedColorOrder::RBG: return "RBG";
        case LedColorOrder::GRB: return "GRB";
        case LedColorOrder::GBR: return "GBR";
        case LedColorOrder::BRG: return "BRG";
        case LedColorOrder::BGR: return "BGR";
        default: return nullptr;
    }
}

bool parseLedProtocolApiName(const String &value, LedProtocol &protocol) {
    if (value != "ws2811_800k") return false;
    protocol = LedProtocol::WS2811_800K;
    return true;
}

bool parseLedColorOrderApiName(const String &value, LedColorOrder &colorOrder) {
    if (value == "RGB") colorOrder = LedColorOrder::RGB;
    else if (value == "RBG") colorOrder = LedColorOrder::RBG;
    else if (value == "GRB") colorOrder = LedColorOrder::GRB;
    else if (value == "GBR") colorOrder = LedColorOrder::GBR;
    else if (value == "BRG") colorOrder = LedColorOrder::BRG;
    else if (value == "BGR") colorOrder = LedColorOrder::BGR;
    else return false;
    return true;
}

void skipHardwareJsonWhitespace(const String &value, int &position) {
    while (position < value.length() &&
           (value[position] == ' ' || value[position] == '\t' ||
            value[position] == '\n' || value[position] == '\r')) {
        position++;
    }
}

void trimHardwareJsonWhitespace(String &value) {
    int first = 0;
    skipHardwareJsonWhitespace(value, first);
    int last = value.length();
    while (last > first &&
           (value[last - 1] == ' ' || value[last - 1] == '\t' ||
            value[last - 1] == '\n' || value[last - 1] == '\r')) {
        last--;
    }
    if (first > 0) value.remove(0, first);
    value.remove(last - first);
}

bool parseHardwareConfigPayload(const String &body, HardwareConfig &hardware) {
    if (body.length() < 2 || body.length() > 256 || body[0] != '{' ||
        body[body.length() - 1] != '}') {
        return false;
    }

    bool rowsSeen = false;
    bool colsSeen = false;
    bool portsSeen = false;
    bool protocolSeen = false;
    bool orderSeen = false;
    int rows = 0;
    int cols = 0;
    int activePortCount = 0;
    LedProtocol protocol = LedProtocol::WS2811_800K;
    LedColorOrder colorOrder = LedColorOrder::BRG;
    int position = 1;

    skipHardwareJsonWhitespace(body, position);
    while (position < body.length() - 1) {
        if (body[position] != '"') return false;
        const int keyEnd = body.indexOf('"', position + 1);
        if (keyEnd < 0) return false;
        const String key = body.substring(position + 1, keyEnd);
        position = keyEnd + 1;

        skipHardwareJsonWhitespace(body, position);
        if (position >= body.length() || body[position] != ':') return false;
        position++;
        skipHardwareJsonWhitespace(body, position);

        if (key == "ledProtocol" || key == "colorOrder") {
            if (position >= body.length() || body[position] != '"') return false;
            const int valueEnd = body.indexOf('"', position + 1);
            if (valueEnd < 0) return false;
            const String value = body.substring(position + 1, valueEnd);
            position = valueEnd + 1;

            if (key == "ledProtocol") {
                if (protocolSeen || !parseLedProtocolApiName(value, protocol)) return false;
                protocolSeen = true;
            } else {
                if (orderSeen || !parseLedColorOrderApiName(value, colorOrder)) return false;
                orderSeen = true;
            }
        } else {
            const int valueStart = position;
            if (position < body.length() && body[position] == '-') position++;
            const int digitStart = position;
            while (position < body.length() && isdigit((unsigned char)body[position])) position++;
            if (position == digitStart) return false;
            if (body[digitStart] == '0' && position > digitStart + 1) return false;

            int parsed = 0;
            if (!parseStrictInt(body.substring(valueStart, position), parsed)) return false;
            if (key == "rows") {
                if (rowsSeen) return false;
                rows = parsed;
                rowsSeen = true;
            } else if (key == "cols") {
                if (colsSeen) return false;
                cols = parsed;
                colsSeen = true;
            } else if (key == "activePortCount") {
                if (portsSeen) return false;
                activePortCount = parsed;
                portsSeen = true;
            } else {
                return false;
            }
        }

        skipHardwareJsonWhitespace(body, position);
        if (position == body.length() - 1) break;
        if (body[position] != ',') return false;
        position++;
        skipHardwareJsonWhitespace(body, position);
        if (position >= body.length() - 1) return false;
    }

    if (position != body.length() - 1 || !rowsSeen || !colsSeen ||
        !portsSeen || !protocolSeen || !orderSeen || rows < 1 || cols < 1 ||
        rows > static_cast<int>(MAX_LED_COUNT_PER_OUTPUT) ||
        cols > static_cast<int>(MAX_LED_COUNT_PER_OUTPUT) ||
        activePortCount < 1 ||
        activePortCount > static_cast<int>(MAX_PORT_COUNT)) {
        return false;
    }

    hardware = HardwareConfig{
        static_cast<uint16_t>(rows),
        static_cast<uint16_t>(cols),
        static_cast<uint8_t>(activePortCount),
        protocol,
        colorOrder,
    };
    return hardware_config_validate(hardware);
}

bool consumeHardwareJsonCharacter(
    const String &body,
    int &position,
    char expected) {
    skipHardwareJsonWhitespace(body, position);
    if (position >= body.length() || body[position] != expected) return false;
    position++;
    return true;
}

bool parseHardwareJsonString(
    const String &body,
    int &position,
    String &value) {
    skipHardwareJsonWhitespace(body, position);
    if (position >= body.length() || body[position] != '"') return false;
    const int valueStart = ++position;
    while (position < body.length() && body[position] != '"') {
        const unsigned char character =
            static_cast<unsigned char>(body[position]);
        if (character < 0x20 || body[position] == '\\') return false;
        position++;
    }
    if (position >= body.length()) return false;
    value = body.substring(valueStart, position);
    position++;
    return true;
}

bool parseHardwareJsonInteger(
    const String &body,
    int &position,
    int &value) {
    skipHardwareJsonWhitespace(body, position);
    const int valueStart = position;
    if (position < body.length() && body[position] == '-') position++;
    const int digitStart = position;
    while (position < body.length() &&
           isdigit(static_cast<unsigned char>(body[position]))) {
        position++;
    }
    if (position == digitStart) return false;
    if (body[digitStart] == '0' && position > digitStart + 1) return false;
    return parseStrictInt(body.substring(valueStart, position), value);
}

bool parseHardwareJsonBoolean(
    const String &body,
    int &position,
    bool &value) {
    skipHardwareJsonWhitespace(body, position);
    if (body.substring(position, position + 4) == "true") {
        position += 4;
        value = true;
        return true;
    }
    if (body.substring(position, position + 5) == "false") {
        position += 5;
        value = false;
        return true;
    }
    return false;
}

bool parseHardwareOutputDescriptor(
    const String &body,
    int &position,
    size_t expectedIndex,
    MatrixStateV3Output &parsedOutput) {
    if (!consumeHardwareJsonCharacter(body, position, '{')) return false;

    bool indexSeen = false;
    bool enabledSeen = false;
    bool rowsSeen = false;
    bool colsSeen = false;
    bool topologySeen = false;
    bool protocolSeen = false;
    bool orderSeen = false;
    int index = -1;
    int rows = -1;
    int cols = -1;
    int topology = -1;
    bool enabled = false;
    LedProtocol protocol = LedProtocol::WS2811_800K;
    LedColorOrder colorOrder = LedColorOrder::BRG;

    skipHardwareJsonWhitespace(body, position);
    while (position < body.length() && body[position] != '}') {
        String key;
        if (!parseHardwareJsonString(body, position, key) ||
            !consumeHardwareJsonCharacter(body, position, ':')) {
            return false;
        }

        if (key == "index") {
            if (indexSeen ||
                !parseHardwareJsonInteger(body, position, index)) {
                return false;
            }
            indexSeen = true;
        } else if (key == "enabled") {
            if (enabledSeen ||
                !parseHardwareJsonBoolean(body, position, enabled)) {
                return false;
            }
            enabledSeen = true;
        } else if (key == "rows") {
            if (rowsSeen ||
                !parseHardwareJsonInteger(body, position, rows)) {
                return false;
            }
            rowsSeen = true;
        } else if (key == "cols") {
            if (colsSeen ||
                !parseHardwareJsonInteger(body, position, cols)) {
                return false;
            }
            colsSeen = true;
        } else if (key == "topology") {
            if (topologySeen ||
                !parseHardwareJsonInteger(body, position, topology)) {
                return false;
            }
            topologySeen = true;
        } else if (key == "ledProtocol" || key == "colorOrder") {
            String value;
            if (!parseHardwareJsonString(body, position, value)) return false;
            if (key == "ledProtocol") {
                if (protocolSeen ||
                    !parseLedProtocolApiName(value, protocol)) {
                    return false;
                }
                protocolSeen = true;
            } else {
                if (orderSeen ||
                    !parseLedColorOrderApiName(value, colorOrder)) {
                    return false;
                }
                orderSeen = true;
            }
        } else {
            return false;
        }

        skipHardwareJsonWhitespace(body, position);
        if (position < body.length() && body[position] == '}') break;
        if (!consumeHardwareJsonCharacter(body, position, ',')) return false;
        skipHardwareJsonWhitespace(body, position);
        if (position >= body.length() || body[position] == '}') return false;
    }

    if (!consumeHardwareJsonCharacter(body, position, '}') ||
        !indexSeen || !enabledSeen || !rowsSeen || !colsSeen ||
        !topologySeen || !protocolSeen || !orderSeen ||
        index != static_cast<int>(expectedIndex) ||
        rows < 0 || rows > static_cast<int>(MAX_LED_COUNT_PER_OUTPUT) ||
        cols < 0 || cols > static_cast<int>(MAX_LED_COUNT_PER_OUTPUT) ||
        topology < 0 || topology > MATRIX_TOPOLOGY_MAX) {
        return false;
    }
    if ((enabled && (rows == 0 || cols == 0)) ||
        (!enabled && (rows != 0 || cols != 0))) {
        return false;
    }

    parsedOutput = MatrixStateV3Output{
        static_cast<uint16_t>(rows),
        static_cast<uint16_t>(cols),
        static_cast<uint8_t>(enabled ? 1 : 0),
        static_cast<uint8_t>(topology),
        static_cast<uint8_t>(protocol),
        static_cast<uint8_t>(colorOrder),
    };
    return true;
}

bool validateMultiOutputHardwareConfig(
    const MultiOutputHardwareConfig &hardware) {
    if (hardware.activePortCount < 1 ||
        hardware.activePortCount > MAX_PORT_COUNT) {
        return false;
    }

    size_t totalLedCount = 0;
    for (size_t outputIndex = 0; outputIndex < LED_OUTPUT_COUNT;
         outputIndex++) {
        const MatrixStateV3Output &output = hardware.outputs[outputIndex];
        if (output.enabled > 1 || output.topology > MATRIX_TOPOLOGY_MAX ||
            !hardware_config_led_output_supported(
                static_cast<LedProtocol>(output.ledProtocol),
                static_cast<LedColorOrder>(output.colorOrder))) {
            return false;
        }
        if (output.enabled == 0) {
            if (output.rows != 0 || output.cols != 0) return false;
            continue;
        }
        if (output.rows == 0 || output.cols == 0 ||
            output.rows > MAX_LED_COUNT_PER_OUTPUT / output.cols) {
            return false;
        }
        const size_t length =
            static_cast<size_t>(output.rows) * output.cols;
        if (length > MAX_TOTAL_LED_COUNT - totalLedCount) return false;
        totalLedCount += length;
    }
    return true;
}

bool parseMultiOutputHardwareConfigPayload(
    const String &body,
    MultiOutputHardwareConfig &hardware) {
    if (body.length() < 2 || body.length() > HARDWARE_CONFIG_BODY_LIMIT) {
        return false;
    }

    MultiOutputHardwareConfig parsed = {};
    bool portsSeen = false;
    bool outputsSeen = false;
    int activePortCount = 0;
    int position = 0;
    if (!consumeHardwareJsonCharacter(body, position, '{')) return false;

    skipHardwareJsonWhitespace(body, position);
    while (position < body.length() && body[position] != '}') {
        String key;
        if (!parseHardwareJsonString(body, position, key) ||
            !consumeHardwareJsonCharacter(body, position, ':')) {
            return false;
        }

        if (key == "activePortCount") {
            if (portsSeen ||
                !parseHardwareJsonInteger(
                    body, position, activePortCount)) {
                return false;
            }
            portsSeen = true;
        } else if (key == "outputs") {
            if (outputsSeen ||
                !consumeHardwareJsonCharacter(body, position, '[')) {
                return false;
            }
            for (size_t outputIndex = 0; outputIndex < LED_OUTPUT_COUNT;
                 outputIndex++) {
                if (outputIndex > 0 &&
                    !consumeHardwareJsonCharacter(body, position, ',')) {
                    return false;
                }
                if (!parseHardwareOutputDescriptor(
                        body,
                        position,
                        outputIndex,
                        parsed.outputs[outputIndex])) {
                    return false;
                }
            }
            if (!consumeHardwareJsonCharacter(body, position, ']')) {
                return false;
            }
            outputsSeen = true;
        } else {
            return false;
        }

        skipHardwareJsonWhitespace(body, position);
        if (position < body.length() && body[position] == '}') break;
        if (!consumeHardwareJsonCharacter(body, position, ',')) return false;
        skipHardwareJsonWhitespace(body, position);
        if (position >= body.length() || body[position] == '}') return false;
    }

    if (!consumeHardwareJsonCharacter(body, position, '}')) return false;
    skipHardwareJsonWhitespace(body, position);
    if (position != body.length() || !portsSeen || !outputsSeen ||
        activePortCount < 1 ||
        activePortCount > static_cast<int>(MAX_PORT_COUNT)) {
        return false;
    }

    parsed.activePortCount = static_cast<uint8_t>(activePortCount);
    if (!validateMultiOutputHardwareConfig(parsed)) return false;
    hardware = parsed;
    return true;
}

bool parseMatrixCoordinate(
    const String &coordinate,
    const MatrixStateV3Output &output,
    int &x,
    int &y) {
    const int dash = coordinate.indexOf('-');
    if (dash <= 0 || dash != coordinate.lastIndexOf('-') || dash >= coordinate.length() - 1) return false;
    return parseStrictInt(coordinate.substring(0, dash), x) &&
        parseStrictInt(coordinate.substring(dash + 1), y) &&
        x >= 0 && x < output.cols && y >= 0 && y < output.rows;
}

bool parseZoneMapPayload(
    const String &body,
    MatrixStateV3 &state,
    size_t outputIndex,
    const MatrixStateV3OutputLayout &outputLayout) {
    const MatrixStateV3Output &output = state.outputs[outputIndex];
    if (outputLayout.length == 0 ||
        state.activePortCount < 1 || state.activePortCount > MAX_PORT_COUNT ||
        body.length() < 2 ||
        body.length() > (2 + (outputLayout.length * 24U)) || body[0] != '{' ||
        body[body.length() - 1] != '}') {
        return false;
    }

    for (size_t localIndex = 0; localIndex < outputLayout.length;
         localIndex++) {
        state.zoneMap[outputLayout.offset + localIndex] = ZONE_ID_OFF;
    }
    uint8_t seen[(MAX_LED_COUNT_PER_OUTPUT + 7U) / 8U] = {};
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
        if (!parseMatrixCoordinate(coordinate, output, x, y) ||
            !parseStrictInt(zoneValue, zoneId) ||
            zoneId < ZONE_ID_OFF || zoneId > ZONE_ID_MAX || !zone_id_is_valid((uint8_t)zoneId) ||
            (zone_is_port((uint8_t)zoneId) && zoneId > highestActivePortZone)) {
            return false;
        }
        const size_t localIndex = (size_t)y * output.cols + x;
        const size_t globalIndex = outputLayout.offset + localIndex;
        const uint8_t bit = (uint8_t)(1U << (localIndex % 8U));
        if ((seen[localIndex / 8U] & bit) != 0 ||
            entryCount >= outputLayout.length) {
            return false;
        }
        seen[localIndex / 8U] |= bit;
        state.zoneMap[globalIndex] = (uint8_t)zoneId;
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
    const MatrixStateV3 &state,
    size_t outputIndex,
    const MatrixStateV3OutputLayout &outputLayout,
    MatrixStateV3StoredLayer &layer) {
    const MatrixStateV3Output &output = state.outputs[outputIndex];
    if (outputLayout.length == 0) return false;

    String body = payload;
    body.trim();
    if (body.length() < 2 ||
        body.length() > (2 + (outputLayout.length * 32U)) ||
        body[0] != '{' || body[body.length() - 1] != '}') {
        return false;
    }

    for (size_t localIndex = 0; localIndex < outputLayout.length;
         localIndex++) {
        clearStoredLayerPixel(layer, outputLayout.offset + localIndex);
    }
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
        if (!parseMatrixCoordinate(coordinate, output, x, y)) return false;
        const size_t localIndex = (size_t)y * output.cols + x;
        const size_t globalIndex = outputLayout.offset + localIndex;
        const uint8_t bit = (uint8_t)(1U << (globalIndex % 8U));
        if ((layer.present[globalIndex / 8U] & bit) != 0 ||
            entryCount >= outputLayout.length) {
            return false;
        }

        uint8_t parsedColor[3] = {};
        parseHexColor(colorText, parsedColor);
        layer.present[globalIndex / 8U] |= bit;
        memcpy(&layer.rgb[globalIndex * 3U], parsedColor, sizeof(parsedColor));
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

bool parseStatusJsonString(
    const String &body,
    int &position,
    String &value) {
    if (position >= body.length() || body[position] != '"') return false;
    const int valueStart = ++position;
    while (position < body.length()) {
        const unsigned char current = (unsigned char)body[position];
        if (current == '"') {
            value = body.substring(valueStart, position);
            position++;
            return true;
        }
        if (current == '\\' || current < 0x20U) return false;
        position++;
    }
    return false;
}

bool parseStatusJsonObject(
    const String &body,
    int &position,
    String &value) {
    if (position >= body.length() || body[position] != '{') return false;
    const int objectStart = position;
    int depth = 0;
    bool inString = false;
    bool escaped = false;

    while (position < body.length()) {
        const unsigned char current = (unsigned char)body[position];
        if (inString) {
            if (escaped) escaped = false;
            else if (current == '\\') escaped = true;
            else if (current == '"') inString = false;
            else if (current < 0x20U) return false;
        } else if (current == '"') {
            inString = true;
        } else if (current == '{') {
            depth++;
        } else if (current == '}') {
            depth--;
            if (depth == 0) {
                position++;
                value = body.substring(objectStart, position);
                return true;
            }
            if (depth < 0) return false;
        }
        position++;
    }
    return false;
}

bool parseStatusSavePayload(
    const String &body,
    String &status,
    String &lightMode,
    String &colorsJson,
    bool &hasBaseColor,
    String &baseColor) {
    if (body.length() < 2 || body[0] != '{' ||
        body[body.length() - 1] != '}') {
        return false;
    }

    bool statusSeen = false;
    bool lightModeSeen = false;
    bool colorsSeen = false;
    hasBaseColor = false;
    int position = 1;
    skipJsonWhitespace(body, position);
    if (position == body.length() - 1) return false;

    while (position < body.length() - 1) {
        String fieldName;
        if (!parseStatusJsonString(body, position, fieldName)) return false;
        skipJsonWhitespace(body, position);
        if (position >= body.length() || body[position] != ':') return false;
        position++;
        skipJsonWhitespace(body, position);

        if (fieldName == "status") {
            if (statusSeen || !parseStatusJsonString(body, position, status)) {
                return false;
            }
            statusSeen = true;
        } else if (fieldName == "lightMode") {
            if (lightModeSeen ||
                !parseStatusJsonString(body, position, lightMode)) {
                return false;
            }
            lightModeSeen = true;
        } else if (fieldName == "baseColor") {
            if (hasBaseColor ||
                !parseStatusJsonString(body, position, baseColor)) {
                return false;
            }
            hasBaseColor = true;
        } else if (fieldName == "colors") {
            if (colorsSeen ||
                !parseStatusJsonObject(body, position, colorsJson)) {
                return false;
            }
            colorsSeen = true;
        } else {
            return false;
        }

        skipJsonWhitespace(body, position);
        if (position == body.length() - 1) {
            return statusSeen && lightModeSeen && colorsSeen;
        }
        if (position >= body.length() || body[position] != ',') return false;
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

bool rejectMatrixSaveForRestart() {
    if (rejectSaveForControllerRestart()) return true;
    if (hardwareConfigRestartRequired) {
        server.send(409, "text/plain", "RESTART_PENDING");
        return true;
    }
    return false;
}

bool prepareSelectedOutputSave(
    bool requireEnabled,
    size_t &outputIndex,
    MatrixStateV3OutputLayout &selectedLayout) {
    if (rejectMatrixSaveForRestart()) return false;

    bool outputSpecified = false;
    int parsedOutputIndex = 0;
    for (int argIndex = 0; argIndex < server.args(); argIndex++) {
        if (server.argName(argIndex) != "output") continue;
        if (outputSpecified ||
            !parseStrictInt(server.arg(argIndex), parsedOutputIndex) ||
            parsedOutputIndex < 0 ||
            parsedOutputIndex >= static_cast<int>(LED_OUTPUT_COUNT)) {
            server.send(400, "text/plain", "BAD_OUTPUT_INDEX");
            return false;
        }
        outputSpecified = true;
    }

    if (!snapshotMatrixState()) return false;

    MatrixStateV3OutputLayout outputLayout[LED_OUTPUT_COUNT] = {};
    size_t totalLedCount = 0;
    if (!matrix_state_v3_calculate_output_layout(
            matrixStateCandidate, outputLayout, totalLedCount)) {
        server.send(500, "text/plain", "STATE_INVALID");
        return false;
    }

    if (!outputSpecified && !currentUiSupportsMatrixState(matrixStateCandidate)) {
        server.send(409, "text/plain", "OUTPUT_REQUIRED");
        return false;
    }

    outputIndex = static_cast<size_t>(parsedOutputIndex);
    selectedLayout = outputLayout[outputIndex];
    const MatrixStateV3Output &output = matrixStateCandidate.outputs[outputIndex];
    if (requireEnabled &&
        (output.enabled != 1 || selectedLayout.length == 0)) {
        server.send(409, "text/plain", "OUTPUT_DISABLED");
        return false;
    }
    return true;
}

bool saveAndApplyMatrixState(const char *saveError) {
    if (matrix_state_storage_validate_v3(matrixStateCandidate) !=
        MatrixStateValidationResult::VALID) {
        Serial.println("Matrix state candidate validation failed");
        server.send(500, "text/plain", saveError);
        return false;
    }

    const MatrixStateSaveResult saveResult =
        matrix_state_storage_save_v3(matrixStateCandidate);
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
    const MatrixStateV3Output &output = matrixStateCandidate.outputs[0];
    String sizeStr = String(output.cols) + ":" + String(output.rows);
    server.send(200, "text/plain", sizeStr);
}

void handleGetConfig() {
    if (!requireHttpAuth()) return;

    bool outputSpecified = false;
    int selectedOutputIndex = 0;
    for (int argIndex = 0; argIndex < server.args(); argIndex++) {
        if (server.argName(argIndex) != "output") continue;
        if (outputSpecified ||
            !parseStrictInt(server.arg(argIndex), selectedOutputIndex) ||
            selectedOutputIndex < 0 ||
            selectedOutputIndex >= static_cast<int>(LED_OUTPUT_COUNT)) {
            server.send(400, "text/plain", "BAD_OUTPUT_INDEX");
            return;
        }
        outputSpecified = true;
    }

    if (!outputSpecified && matrixUiRequiresMultiOutput) {
        server.send(409, "text/plain", "MULTI_OUTPUT_UI_REQUIRED");
        return;
    }
    if (!snapshotMatrixState()) return;

    MatrixStateV3OutputLayout outputLayout[LED_OUTPUT_COUNT] = {};
    size_t totalLedCount = 0;
    if (!matrix_state_v3_calculate_output_layout(
            matrixStateCandidate, outputLayout, totalLedCount)) {
        server.send(500, "text/plain", "STATE_INVALID");
        return;
    }
    const size_t outputIndex = static_cast<size_t>(selectedOutputIndex);
    const MatrixStateV3Output &output =
        matrixStateCandidate.outputs[outputIndex];
    const MatrixStateV3OutputLayout &selectedLayout =
        outputLayout[outputIndex];

    String json;
    if (!json.reserve(GET_CONFIG_CHUNK_RESERVE)) {
        server.send(500, "text/plain", "RESPONSE_BUFFER_UNAVAILABLE");
        return;
    }

    json += "{\"topo\":";
    json += String((int)output.topology);
    json += ",\"b_ports\":";
    json += String((int)matrixStateCandidate.portBrightness);
    json += ",\"c_wait\":\"";
    json += rgbToHex(matrixStateCandidate.waitingColor);
    json += "\",\"c_charge\":\"";
    json += rgbToHex(matrixStateCandidate.chargingColor);
    json += "\",\"c_error\":\"";
    json += rgbToHex(matrixStateCandidate.errorColor);
    json += "\",\"statusLightModes\":{\"waiting\":\"";
    json += statusLightModeApiName(matrix_state_resolve_status_light_mode(
        MATRIX_STATE_LAYER_WAITING,
        matrixStateCandidate.statusLightModes[MATRIX_STATE_LAYER_WAITING]));
    json += "\",\"charging\":\"";
    json += statusLightModeApiName(matrix_state_resolve_status_light_mode(
        MATRIX_STATE_LAYER_CHARGING,
        matrixStateCandidate.statusLightModes[MATRIX_STATE_LAYER_CHARGING]));
    json += "\",\"error\":\"";
    json += statusLightModeApiName(matrix_state_resolve_status_light_mode(
        MATRIX_STATE_LAYER_ERROR,
        matrixStateCandidate.statusLightModes[MATRIX_STATE_LAYER_ERROR]));
    json += "\"},\"activePortCount\":";
    json += String((int)matrixStateCandidate.activePortCount);
    json += ",\"selectedOutputIndex\":";
    json += String(selectedOutputIndex);
    json += ",\"outputs\":[";
    for (size_t i = 0; i < LED_OUTPUT_COUNT; i++) {
        if (i > 0) json += ",";
        const MatrixStateV3Output &listedOutput =
            matrixStateCandidate.outputs[i];
        json += "{\"index\":";
        json += String((int)i);
        json += ",\"name\":\"LED";
        json += String((int)i + 1);
        json += "\",\"enabled\":";
        json += listedOutput.enabled ? "true" : "false";
        json += ",\"rows\":";
        json += String((int)listedOutput.rows);
        json += ",\"cols\":";
        json += String((int)listedOutput.cols);
        json += ",\"total\":";
        json += String((int)outputLayout[i].length);
        json += ",\"topology\":";
        json += String((int)listedOutput.topology);
        json += ",\"ledProtocol\":\"";
        json += ledProtocolApiName(
            static_cast<LedProtocol>(listedOutput.ledProtocol));
        json += "\",\"colorOrder\":\"";
        json += ledColorOrderApiName(
            static_cast<LedColorOrder>(listedOutput.colorOrder));
        json += "\"}";
    }
    json += "]";
    json += ",\"totalLedCount\":";
    json += String((unsigned int)totalLedCount);
    json += ",\"maxTotalLedCount\":";
    json += String((unsigned int)MAX_TOTAL_LED_COUNT);
    json += ",\"matrix\":{\"cols\":";
    json += String((int)output.cols);
    json += ",\"rows\":";
    json += String((int)output.rows);
    json += ",\"total\":";
    json += String((int)selectedLayout.length);
    json += ",\"topology\":";
    json += String((int)output.topology);
    json += "}";
    json += ",\"hardware\":{\"rows\":";
    json += String((int)output.rows);
    json += ",\"cols\":";
    json += String((int)output.cols);
    json += ",\"activePortCount\":";
    json += String((int)matrixStateCandidate.activePortCount);
    json += ",\"ledProtocol\":\"";
    json += ledProtocolApiName(
        static_cast<LedProtocol>(output.ledProtocol));
    json += "\",\"colorOrder\":\"";
    json += ledColorOrderApiName(
        static_cast<LedColorOrder>(output.colorOrder));
    json += "\",\"maxLedCount\":";
    json += String((int)MAX_LED_COUNT_PER_OUTPUT);
    json += "}";

    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "application/json", json);
    json.remove(0);

    json += ",\"layers\":{\"wait\":";
    appendStoredLayerJson(
        json,
        matrixStateCandidate,
        MATRIX_STATE_LAYER_WAITING,
        outputIndex,
        selectedLayout);
    sendAndClearConfigChunk(json);

    json += ",\"charge\":";
    appendStoredLayerJson(
        json,
        matrixStateCandidate,
        MATRIX_STATE_LAYER_CHARGING,
        outputIndex,
        selectedLayout);
    sendAndClearConfigChunk(json);

    json += ",\"err\":";
    appendStoredLayerJson(
        json,
        matrixStateCandidate,
        MATRIX_STATE_LAYER_ERROR,
        outputIndex,
        selectedLayout);
    json += "}";
    sendAndClearConfigChunk(json);

    appendHardwareMapJson(
        json, matrixStateCandidate, outputIndex, selectedLayout);
    sendAndClearConfigChunk(json);

    appendZoneMetadataJson(json, matrixStateCandidate, outputIndex);
    sendAndClearConfigChunk(json);

    for (size_t i = 0; i < FREE_ZONE_COUNT; i++) {
        if (i == 0) json += ",\"free_zones\":[";
        else json += ",";
        appendFreeZoneConfigJson(
            json, matrixStateCandidate, outputIndex, selectedLayout, i);
        sendAndClearConfigChunk(json);
    }

    json += "]}";
    sendAndClearConfigChunk(json);
    server.sendContent("");
}

void handleSaveHardwareConfig() {
    if (!requireHttpAuth()) return;
    if (rejectMatrixSaveForRestart()) return;

    String body = server.arg("plain");
    if (body.length() > HARDWARE_CONFIG_BODY_LIMIT) {
        server.send(400, "text/plain", "BAD_CONFIG");
        return;
    }
    trimHardwareJsonWhitespace(body);
    MultiOutputHardwareConfig requestedOutputs = {};
    HardwareConfig requestedHardware = {};
    const bool multiOutputPayload =
        parseMultiOutputHardwareConfigPayload(body, requestedOutputs);
    const bool legacyPayload = !multiOutputPayload &&
        parseHardwareConfigPayload(body, requestedHardware);
    if (!multiOutputPayload && !legacyPayload) {
        server.send(400, "text/plain", "BAD_CONFIG");
        return;
    }
    if (!snapshotMatrixState()) return;

    if (legacyPayload &&
        !currentUiSupportsMatrixState(matrixStateCandidate)) {
        server.send(
            409, "text/plain", "MULTI_OUTPUT_CONFIG_REQUIRED");
        return;
    }

    bool spatialChange = false;
    bool protocolOrOrderChange = false;
    bool topologyChange = false;
    const uint8_t requestedActivePortCount = multiOutputPayload
        ? requestedOutputs.activePortCount
        : requestedHardware.activePortCount;
    spatialChange =
        matrixStateCandidate.activePortCount != requestedActivePortCount;

    if (multiOutputPayload) {
        for (size_t outputIndex = 0; outputIndex < LED_OUTPUT_COUNT;
             outputIndex++) {
            const MatrixStateV3Output &currentOutput =
                matrixStateCandidate.outputs[outputIndex];
            const MatrixStateV3Output &requestedOutput =
                requestedOutputs.outputs[outputIndex];
            spatialChange = spatialChange ||
                currentOutput.enabled != requestedOutput.enabled ||
                currentOutput.rows != requestedOutput.rows ||
                currentOutput.cols != requestedOutput.cols;
            protocolOrOrderChange = protocolOrOrderChange ||
                currentOutput.ledProtocol != requestedOutput.ledProtocol ||
                currentOutput.colorOrder != requestedOutput.colorOrder;
            topologyChange = topologyChange ||
                currentOutput.topology != requestedOutput.topology;
            matrixStateCandidate.outputs[outputIndex] = requestedOutput;
        }
    } else {
        MatrixStateV3Output &led1Output = matrixStateCandidate.outputs[0];
        spatialChange = spatialChange ||
            led1Output.rows != requestedHardware.rows ||
            led1Output.cols != requestedHardware.cols;
        protocolOrOrderChange =
            led1Output.ledProtocol !=
                static_cast<uint8_t>(requestedHardware.ledProtocol) ||
            led1Output.colorOrder !=
                static_cast<uint8_t>(requestedHardware.colorOrder);
        led1Output.enabled = 1;
        led1Output.rows = requestedHardware.rows;
        led1Output.cols = requestedHardware.cols;
        led1Output.ledProtocol =
            static_cast<uint8_t>(requestedHardware.ledProtocol);
        led1Output.colorOrder =
            static_cast<uint8_t>(requestedHardware.colorOrder);
    }

    matrixStateCandidate.activePortCount = requestedActivePortCount;
    if (spatialChange) {
        matrix_state_storage_clear_v3_spatial_data(matrixStateCandidate);
    }

    if (matrix_state_storage_validate_v3(matrixStateCandidate) !=
        MatrixStateValidationResult::VALID) {
        Serial.println("Hardware configuration produced an invalid matrix state");
        server.send(400, "text/plain", "BAD_CONFIG");
        return;
    }

    const bool restartRequired =
        spatialChange || protocolOrOrderChange;
    if (!restartRequired && !topologyChange) {
        server.send(200, "text/plain", "NO_CHANGE");
        return;
    }

    const MatrixStateSaveResult saveResult =
        matrix_state_storage_save_v3(matrixStateCandidate);
    if (saveResult != MatrixStateSaveResult::SAVED) {
        Serial.printf("Hardware configuration save failed: %s\n",
                      matrixSaveResultName(saveResult));
        server.send(500, "text/plain", "SAVE_FAILED");
        return;
    }

    if (!restartRequired) {
        if (!led_apply_matrix_state_safe(matrixStateCandidate)) {
            Serial.println(
                "Hardware configuration saved, but runtime apply failed; "
                "saved state will load after reboot");
            server.send(
                500, "text/plain", "APPLY_FAILED_REBOOT_REQUIRED");
            return;
        }
        server.send(200, "text/plain", "OK");
        return;
    }

    hardwareConfigRestartRequired = true;
    if (!led_prepare_for_restart()) {
        Serial.println(
            "Hardware configuration saved, but LED restart preparation failed; "
            "manual reboot required");
        server.send(500, "text/plain", "SAVED_RESTART_REQUIRED");
        return;
    }

    hardwareConfigRestartScheduled = true;
    hardwareConfigRestartAt = millis() + HARDWARE_CONFIG_RESTART_DELAY_MS;
    server.send(200, "text/plain", "REBOOT_REQUIRED");
}

void handleSaveZones() {
    if (!requireHttpAuth()) return;
    size_t outputIndex = 0;
    MatrixStateV3OutputLayout outputLayout = {};
    if (!prepareSelectedOutputSave(true, outputIndex, outputLayout)) return;

    String body = server.arg("plain");
    body.trim();
    memcpy(previousZoneMap, matrixStateCandidate.zoneMap, sizeof(previousZoneMap));
    pruneFreeCustomLayersForZoneMapSegment(
        matrixStateCandidate, outputLayout);
    if (!parseZoneMapPayload(
            body, matrixStateCandidate, outputIndex, outputLayout)) {
        server.send(400, "text/plain", "BAD_JSON");
        return;
    }

    pruneFreeCustomLayersForZoneMapSegment(
        matrixStateCandidate, outputLayout);
    clearStatusPixelsForChangedZones(
        matrixStateCandidate, previousZoneMap, outputLayout);

    if (!saveAndApplyMatrixState("SAVE_FAILED")) return;
    server.send(200, "text/plain", "OK");
}

void handleSaveTopology() {
    if (!requireHttpAuth()) return;
    size_t outputIndex = 0;
    MatrixStateV3OutputLayout outputLayout = {};
    if (!prepareSelectedOutputSave(false, outputIndex, outputLayout)) return;

    int topology = -1;
    if (server.hasArg("topology")) {
        if (!parseStrictInt(server.arg("topology"), topology)) topology = -1;
    } else {
        const String body = server.arg("plain");
        if (!extractJsonIntField(body, "topology", topology)) topology = -1;
    }

    if (topology < 0 || topology > MATRIX_TOPOLOGY_MAX) {
        server.send(400, "text/plain", "BAD_TOPOLOGY");
        return;
    }

    matrixStateCandidate.outputs[outputIndex].topology = (uint8_t)topology;
    if (!saveAndApplyMatrixState("SAVE_FAILED")) return;
    server.send(200, "text/plain", "OK");
}

void handleSetBright() {
    if (!requireHttpAuth()) return;
    if (rejectMatrixSaveForRestart()) return;

    if (!server.hasArg("type") || !server.hasArg("val")) {
        server.send(400, "text/plain", "MISSING_ARG");
        return;
    }

    const String type = server.arg("type");
    int parsedValue = 0;
    if (!parseStrictInt(server.arg("val"), parsedValue) || parsedValue < 0 || parsedValue > 255) {
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
    size_t outputIndex = 0;
    MatrixStateV3OutputLayout outputLayout = {};
    if (!prepareSelectedOutputSave(true, outputIndex, outputLayout)) return;

    String body = server.arg("plain");
    body.trim();
    if (!body.startsWith("{") || !body.endsWith("}") ||
        body.length() > (2 + (outputLayout.length * 32U) + 128U)) {
        server.send(400, "text/plain", "BAD_JSON");
        return;
    }

    String requestedStatus;
    String requestedLightMode;
    String colorsJson;
    String baseColorText;
    bool hasBaseColor = false;
    if (!parseStatusSavePayload(
            body,
            requestedStatus,
            requestedLightMode,
            colorsJson,
            hasBaseColor,
            baseColorText)) {
        server.send(400, "text/plain", "BAD_JSON");
        return;
    }

    String status;
    if (!canonicalStatusName(requestedStatus, status)) {
        server.send(400, "text/plain", "BAD_STATUS");
        return;
    }

    StatusLightMode lightMode = StatusLightMode::DEFAULT_MODE;
    if (!parseStatusLightModeApiName(requestedLightMode, lightMode)) {
        server.send(400, "text/plain", "BAD_LIGHT_MODE");
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

    uint8_t factoryColor[3] = {};
    if (!matrix_state_storage_get_factory_status_color(
            layerIndex, factoryColor)) {
        server.send(500, "text/plain", "STATE_INVALID");
        return;
    }
    if (hasBaseColor) {
        uint8_t parsedBaseColor[3] = {};
        if (!isValidHexColor(baseColorText)) {
            server.send(400, "text/plain", "BAD_BASE_COLOR");
            return;
        }
        parseHexColor(baseColorText, parsedBaseColor);
        if (memcmp(parsedBaseColor, factoryColor, sizeof(factoryColor)) != 0) {
            server.send(400, "text/plain", "BAD_BASE_COLOR");
            return;
        }
    }

    if (!parseStoredLayerPayload(
            colorsJson,
            matrixStateCandidate,
            outputIndex,
            outputLayout,
            matrixStateCandidate.layers[layerIndex]) ||
        !statusLayerUsesOnlyActivePortZones(
            matrixStateCandidate,
            matrixStateCandidate.layers[layerIndex],
            outputLayout)) {
        server.send(400, "text/plain", "BAD_LAYER");
        return;
    }
    memcpy(fallbackColor, factoryColor, sizeof(factoryColor));
    matrixStateCandidate.statusLightModes[layerIndex] = lightMode;

    if (!saveAndApplyMatrixState("SAVE_LAYER_FAILED")) return;
    server.send(200, "text/plain", "OK");
}

void handleSaveFreeZone() {
    if (!requireHttpAuth()) return;
    size_t outputIndex = 0;
    MatrixStateV3OutputLayout outputLayout = {};
    if (!prepareSelectedOutputSave(true, outputIndex, outputLayout)) return;

    String body = server.arg("plain");
    body.trim();
    if (!body.startsWith("{") || !body.endsWith("}") ||
        body.length() > (2 + (outputLayout.length * 32U) + 512U)) {
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

    MatrixStateStoredFreeZone &nextZone =
        matrixStateCandidate.freeZones[outputIndex][zoneIndex];
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
        if (!extractJsonIntField(body, "brightness", brightness) || brightness < 0 || brightness > 255) {
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
        if (!extractJsonObjectField(body, "customLayer", customLayer) ||
            !parseStoredLayerPayload(
                customLayer,
                matrixStateCandidate,
                outputIndex,
                outputLayout,
                matrixStateCandidate.layers[MATRIX_STATE_LAYER_LOGO + zoneIndex])) {
            server.send(400, "text/plain", "BAD_LAYER");
            return;
        }

        const MatrixStateV3StoredLayer &newLayer =
            matrixStateCandidate.layers[MATRIX_STATE_LAYER_LOGO + zoneIndex];
        for (size_t localIndex = 0; localIndex < outputLayout.length;
             localIndex++) {
            const size_t globalIndex = outputLayout.offset + localIndex;
            if (storedLayerHasPixel(newLayer, globalIndex) &&
                matrixStateCandidate.zoneMap[globalIndex] != nextZone.zoneId) {
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
    if (rejectSaveForControllerRestart()) return;

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
    if (rejectSaveForControllerRestart()) return;
    network_schedule_apply(1500);
    server.send(200, "text/plain", "RECONNECT_SCHEDULED");
}

void handleAuthStatus() {
    if (!requireHttpAuth()) return;

    String json = "{\"enabled\":";
    json += auth_config_enabled() ? "true" : "false";
    json += ",\"username\":\"";
    json += jsonEscape(auth_config_username_string());
    json += "\",\"defaultCredentials\":";
    json += auth_config_uses_default_credentials() ? "true" : "false";
    json += "}";
    server.send(200, "application/json", json);
}

void handleSaveAuth() {
    if (!requireHttpAuth()) return;
    if (rejectSaveForControllerRestart()) return;

    const String enabledValue = server.arg("enabled");
    if (enabledValue != "0" && enabledValue != "1") {
        server.send(400, "text/plain", "INVALID_ENABLED");
        return;
    }
    const bool enabled = enabledValue == "1";
    String username = server.arg("username");
    String password = server.arg("password");
    String confirm = server.hasArg("confirm") ? server.arg("confirm") : server.arg("password_confirm");

    if (enabled) {
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
    }

    String error;
    if (!auth_config_save_settings(enabled, username, password, error)) {
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
    json += ",\"enabledOutputCount\":";
    json += String((int)ledRuntime.enabledOutputCount);
    json += ",\"totalLedCount\":";
    json += String((unsigned int)ledRuntime.totalLedCount);
    json += ",\"lastRefreshAgeMs\":";
    json += uint64ToString(ledRuntime.lastRefreshAgeMs);
    json += "}";
    json += ",\"ledOutputs\":[";

    for (uint8_t i = 0; i < LED_OUTPUT_COUNT; i++) {
        if (i > 0) json += ",";
        json += "{\"name\":\"LED";
        json += String((int)i + 1);
        json += "\",\"gpio\":";
        json += String((int)LED_OUTPUT_PINS[i]);
        json += ",\"enabled\":";
        json += ledRuntime.outputLedCount[i] > 0 ? "true" : "false";
        json += ",\"total\":";
        json += String((unsigned int)ledRuntime.outputLedCount[i]);
        json += "}";
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
    setupServiceButton();
    auth_config_setup();

    matrix_state_storage_set_v3_factory_defaults(matrixStateCandidate);
    const MatrixStateLoadResult matrixLoadResult =
        matrix_state_storage_load_v3(matrixStateCandidate);
    if (matrixLoadResult == MatrixStateLoadResult::LOADED) {
        if (currentUiSupportsMatrixState(matrixStateCandidate)) {
            Serial.println("Matrix state V3 loaded for the current matrix UI");
        } else {
            Serial.println(
                "Matrix state V3 requires the multi-output UI; runtime will "
                "use the stored state while matrix saves remain blocked");
            matrixUiRequiresMultiOutput = true;
        }
    } else {
        Serial.printf("Matrix state V3 %s; using factory defaults without saving\n",
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
    server.on("/save_hardware_config", HTTP_POST, handleSaveHardwareConfig);
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
    server.on("/restart", HTTP_POST, handleControllerRestart);
    server.on("/factory_reset", HTTP_POST, handleControllerFactoryReset);

    server.on("/update", HTTP_POST, []() {
        if (!requireHttpAuth()) return;
        if (controllerRestartPending) {
            server.send(409, "text/plain", "RESTART_PENDING");
            return;
        }
        server.sendHeader("Connection", "close");
        if (otaUpdateSuccess && !Update.hasError()) {
            otaRestartPending = true;
            otaRestartAt = millis() + 1000;
            server.send(200, "text/plain", "OK");
        } else if (otaUpdateError == "RESTART_PENDING") {
            server.send(409, "text/plain", "RESTART_PENDING");
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
            if (restartAlreadyScheduled()) {
                otaUpdateError = "RESTART_PENDING";
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

    const esp_partition_t *runningPartition = esp_ota_get_running_partition();
    esp_ota_img_states_t otaState;
    const esp_err_t otaStateResult =
        esp_ota_get_state_partition(runningPartition, &otaState);
    if (otaStateResult == ESP_OK &&
        otaState == ESP_OTA_IMG_PENDING_VERIFY) {
        otaRollbackVerificationPending = true;
        otaRollbackVerificationStartedAt = millis();
        Serial.println("OTA rollback verification pending for 5 seconds");
    } else if (otaStateResult != ESP_OK) {
        Serial.printf("OTA rollback state check failed: %d\n",
            static_cast<int>(otaStateResult));
    }
}

void loop() {
    pollServiceButton();
    const bool serviceButtonReleased =
        !serviceButtonRawPressed && !serviceButtonStablePressed;
    if (serviceButtonReleased && controllerRestartPending &&
        static_cast<int32_t>(millis() - controllerRestartAt) >= 0) {
        controllerRestartPending = false;
        Serial.flush();
        ESP.restart();
    }
    if (serviceButtonRestartPending) return;

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
    if (serviceButtonReleased && hardwareConfigRestartScheduled &&
        static_cast<int32_t>(millis() - hardwareConfigRestartAt) >= 0) {
        hardwareConfigRestartScheduled = false;
        ESP.restart();
    }
    if (serviceButtonReleased && otaRestartPending && millis() >= otaRestartAt) {
        otaRestartPending = false;
        ESP.restart();
    }
    if (otaRollbackVerificationPending &&
        static_cast<uint32_t>(millis() - otaRollbackVerificationStartedAt) >=
            OTA_ROLLBACK_VERIFY_DELAY_MS) {
        otaRollbackVerificationPending = false;
        const esp_err_t verifyResult =
            esp_ota_mark_app_valid_cancel_rollback();
        if (verifyResult == ESP_OK) {
            Serial.println("OTA firmware confirmed");
        } else {
            Serial.printf("OTA firmware confirmation failed: %d\n",
                static_cast<int>(verifyResult));
        }
    }
}
