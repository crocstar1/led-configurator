#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ctype.h>
#include <string.h>
#include "gui_html.h"
#include "led_manager.h"
#include "matrix_config_storage.h"
#include "usp1_board_config.h"
#include "usp1_inputs.h"
#include "status_mapper.h"
#include "zone_model.h"
#include "network_config.h"
#include "auth_config.h"

WebServer server(80);

bool otaUploadAuthorized = false;
bool otaUpdateSuccess = false;
bool otaRestartPending = false;
String otaUpdateError = "";
unsigned long otaRestartAt = 0;

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

String jsonEscape(String value) {
    value.replace("\\", "\\\\");
    value.replace("\"", "\\\"");
    value.replace("\n", "\\n");
    value.replace("\r", "\\r");
    return value;
}

uint8_t clampByteValue(int value, uint8_t fallback) {
    if (value < 1) return fallback;
    if (value > 255) return 255;
    return (uint8_t)value;
}

String extractJsonStringField(const String &json, const char *fieldName) {
    String pattern = "\"";
    pattern += fieldName;
    pattern += "\"";

    int fieldPos = json.indexOf(pattern);
    if (fieldPos < 0) return "";

    int colonPos = json.indexOf(':', fieldPos + pattern.length());
    if (colonPos < 0) return "";

    int quoteStart = json.indexOf('"', colonPos + 1);
    if (quoteStart < 0) return "";

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

int extractJsonIntField(const String &json, const char *fieldName, int fallback) {
    String pattern = "\"";
    pattern += fieldName;
    pattern += "\"";

    int fieldPos = json.indexOf(pattern);
    if (fieldPos < 0) return fallback;

    int colonPos = json.indexOf(':', fieldPos + pattern.length());
    if (colonPos < 0) return fallback;

    int valueStart = colonPos + 1;
    while (valueStart < json.length() && isspace((unsigned char)json[valueStart])) valueStart++;

    if (json.substring(valueStart, valueStart + 4) == "true") return 1;
    if (json.substring(valueStart, valueStart + 5) == "false") return 0;

    int valueEnd = valueStart;
    if (valueEnd < json.length() && json[valueEnd] == '-') valueEnd++;
    while (valueEnd < json.length() && isdigit((unsigned char)json[valueEnd])) valueEnd++;

    if (valueEnd <= valueStart) return fallback;
    return json.substring(valueStart, valueEnd).toInt();
}

String extractJsonObjectField(const String &json, const char *fieldName) {
    String pattern = "\"";
    pattern += fieldName;
    pattern += "\"";

    int fieldPos = json.indexOf(pattern);
    if (fieldPos < 0) return "{}";

    int colonPos = json.indexOf(':', fieldPos + pattern.length());
    if (colonPos < 0) return "{}";

    int objectStart = json.indexOf('{', colonPos + 1);
    if (objectStart < 0) return "{}";

    int depth = 0;
    for (int i = objectStart; i < json.length(); i++) {
        if (json[i] == '{') depth++;
        if (json[i] == '}') {
            depth--;
            if (depth == 0) {
                return json.substring(objectStart, i + 1);
            }
        }
    }

    return "{}";
}

bool findFirstHexColor(const String &json, String &hexColor) {
    int hashPos = json.indexOf("#");
    while (hashPos >= 0) {
        if (hashPos + 7 <= json.length()) {
            String candidate = json.substring(hashPos, hashPos + 7);
            bool valid = true;
            for (int i = 1; i < 7; i++) {
                char c = candidate[i];
                if (!isxdigit((unsigned char)c)) {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                hexColor = candidate;
                return true;
            }
        }
        hashPos = json.indexOf("#", hashPos + 1);
    }
    return false;
}

FreeZoneMode parseFreeZoneMode(String mode, FreeZoneMode fallback) {
    mode.toLowerCase();
    if (mode == "static") return FREE_ZONE_STATIC;
    if (mode == "custom") return FREE_ZONE_CUSTOM;
    if (mode == "rainbow") return FREE_ZONE_RAINBOW;
    return fallback;
}

void appendHardwareMapJson(String &json) {
    json += ",\"hardware_map\":{";
    bool first = true;

    for (int y = 0; y < VIRTUAL_Y; y++) {
        for (int x = 0; x < MATRIX_X; x++) {
            int index = getLEDIndex(x, y);
            if (index < 0) continue;

            uint8_t zoneId = zoneMap[index];
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

void appendStatusLayersJson(String &json) {
    json += ",\"layers\":{\"wait\":";
    json += matrix_config_load_status_layer("waiting");
    json += ",\"charge\":";
    json += matrix_config_load_status_layer("charging");
    json += ",\"err\":";
    json += matrix_config_load_status_layer("error");
    json += "}";
}

FreeZoneMode currentFreeZoneMode(uint8_t zoneId) {
    const int index = free_zone_index(zoneId);
    if (index >= 0) {
        return freeZoneConfigs[index].mode;
    }

    const ZoneMetadata *metadata = zone_metadata_for(zoneId);
    return metadata ? metadata->defaultMode : FREE_ZONE_STATIC;
}

void appendFreeZoneConfigsJson(String &json) {
    json += ",\"free_zones\":[";

    for (size_t i = 0; i < FREE_ZONE_COUNT; i++) {
        if (i > 0) json += ",";

        const FreeZoneConfig &freeZone = freeZoneConfigs[i];
        json += "{\"zoneId\":";
        json += String((int)freeZone.zoneId);
        json += ",\"enabled\":";
        json += (freeZone.enabled ? "true" : "false");
        json += ",\"mode\":\"";
        json += free_zone_mode_to_string(freeZone.mode);
        json += "\",\"staticColor\":\"";
        json += rgbToHex(freeZone.staticColor);
        json += "\",\"brightness\":";
        json += String((int)freeZone.brightness);
        json += ",\"customLayer\":";
        json += matrix_config_load_free_zone_layer(freeZone.zoneId);
        json += "}";
    }

    json += "]";
}

void appendZoneMetadataJson(String &json) {
    json += ",\"zones\":[";

    for (size_t i = 0; i < ZONE_METADATA_COUNT; i++) {
        if (i > 0) json += ",";

        const ZoneMetadata &zone = ZONE_METADATA[i];
        json += "{\"id\":";
        json += String((int)zone.id);
        json += ",\"name\":\"";
        json += zone.name;
        json += "\",\"type\":\"";
        json += zone_type_to_string(zone.type);
        json += "\",\"enabled\":";
        json += (zone.enabled ? "true" : "false");
        json += ",\"reserved\":";
        json += (zone.reserved ? "true" : "false");

        if (zone.type == ZONE_TYPE_PORT) {
            json += ",\"linked_port\":";
            json += String((int)zone.linkedPort + 1);
        } else {
            json += ",\"linked_port\":null";
        }

        if (zone.type == ZONE_TYPE_FREE) {
            json += ",\"mode\":\"";
            json += free_zone_mode_to_string(currentFreeZoneMode(zone.id));
            json += "\"";
        } else {
            json += ",\"mode\":null";
        }

        json += "}";
    }

    json += "]";
}

bool zoneMapHasRequiredActivePortZones(const uint8_t *zones, size_t zoneCount) {
    if (zones == nullptr || zoneCount != NUM_IC_CHIPS) {
        return false;
    }

    const StatusMapperState &status = status_mapper_get_state();
    const uint8_t activePortCount = status.activePortCount > USP1_MAX_PORT_COUNT
        ? USP1_MAX_PORT_COUNT
        : status.activePortCount;

    for (uint8_t portIndex = 0; portIndex < activePortCount; portIndex++) {
        const uint8_t requiredZoneId = (uint8_t)(ZONE_ID_PORT1 + portIndex);
        bool found = false;

        for (size_t i = 0; i < zoneCount; i++) {
            if (zones[i] == requiredZoneId) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

void handleRoot() {
    if (!requireHttpAuth()) return;
    server.send_P(200, "text/html", PAGE_MAIN); 
}

void handleGetMatrixSize() {
    if (!requireHttpAuth()) return;
    String sizeStr = String(MATRIX_X) + ":" + String(VIRTUAL_Y);
    server.send(200, "text/plain", sizeStr);
}

void handleGetConfig() {
    if (!requireHttpAuth()) return;

    String json;
    json.reserve(7600);
    json += "{\"topo\":";
    json += String((int)cfg.topology);
    json += ",\"b_ports\":";
    json += String((int)cfg.bright_ports);
    json += ",\"c_wait\":\"";
    json += rgbToHex(cfg.color_wait);
    json += "\",\"c_charge\":\"";
    json += rgbToHex(cfg.color_charge);
    json += "\",\"c_error\":\"";
    json += rgbToHex(cfg.color_error);
    json += "\",\"matrix\":{\"cols\":";
    json += String((int)MATRIX_X);
    json += ",\"rows\":";
    json += String((int)VIRTUAL_Y);
    json += ",\"total\":";
    json += String((int)NUM_IC_CHIPS);
    json += ",\"topology\":";
    json += String((int)cfg.topology);
    json += "}";
    appendStatusLayersJson(json);
    appendHardwareMapJson(json);
    appendZoneMetadataJson(json);
    appendFreeZoneConfigsJson(json);
    json += "}";

    server.send(200, "application/json", json);
}

void handleSaveZones() {
    if (!requireHttpAuth()) return;

    String body = server.arg("plain");
    body.trim();
    if (!body.startsWith("{") || !body.endsWith("}")) {
        server.send(400, "text/plain", "BAD_JSON");
        return;
    }

    uint8_t nextZoneMap[NUM_IC_CHIPS] = {};

    int pos = 0;
    while (true) {
        int keyStart = body.indexOf('"', pos);
        if (keyStart < 0) break;

        int keyEnd = body.indexOf('"', keyStart + 1);
        if (keyEnd < 0) break;

        String key = body.substring(keyStart + 1, keyEnd);
        int dashPos = key.indexOf('-');
        if (dashPos < 0) {
            pos = keyEnd + 1;
            continue;
        }

        int colonPos = body.indexOf(':', keyEnd + 1);
        if (colonPos < 0) break;

        int valueStart = colonPos + 1;
        while (valueStart < body.length() && isspace((unsigned char)body[valueStart])) valueStart++;

        String value;
        if (valueStart < body.length() && body[valueStart] == '"') {
            int valueEnd = body.indexOf('"', valueStart + 1);
            if (valueEnd < 0) break;
            value = body.substring(valueStart + 1, valueEnd);
            pos = valueEnd + 1;
        } else {
            int valueEnd = valueStart;
            while (valueEnd < body.length() && isdigit((unsigned char)body[valueEnd])) valueEnd++;
            value = body.substring(valueStart, valueEnd);
            pos = valueEnd;
        }

        int x = key.substring(0, dashPos).toInt();
        int y = key.substring(dashPos + 1).toInt();
        int zoneId = value.toInt();

        if (x >= 0 && x < MATRIX_X && y >= 0 && y < VIRTUAL_Y && zoneId >= 0 && zone_id_is_valid((uint8_t)zoneId)) {
            const int index = getLEDIndex(x, y);
            if (index >= 0 && index < NUM_IC_CHIPS) {
                nextZoneMap[index] = (uint8_t)zoneId;
            }
        }
    }

    if (!zoneMapHasRequiredActivePortZones(nextZoneMap, sizeof(nextZoneMap))) {
        server.send(400, "text/plain", "ACTIVE_PORT_ZONE_EMPTY");
        return;
    }

    if (!matrix_config_save(cfg, nextZoneMap, sizeof(nextZoneMap))) {
        server.send(500, "text/plain", "SAVE_FAILED");
        return;
    }

    led_replace_zone_map_safe(nextZoneMap, sizeof(nextZoneMap));
    led_reload_free_zone_layers_safe();
    server.send(200, "text/plain", "OK");
}

void handleSaveTopology() {
    if (!requireHttpAuth()) return;

    int topology = -1;
    if (server.hasArg("topology")) {
        topology = server.arg("topology").toInt();
    } else {
        const String body = server.arg("plain");
        topology = extractJsonIntField(body, "topology", -1);
    }

    if (topology < 0 || topology > 3) {
        server.send(400, "text/plain", "BAD_TOPOLOGY");
        return;
    }

    cfg.topology = (uint8_t)topology;
    if (!matrix_config_save(cfg, zoneMap, sizeof(zoneMap))) {
        server.send(500, "text/plain", "SAVE_FAILED");
        return;
    }

    led_reload_status_layers_safe();
    led_reload_free_zone_layers_safe();
    led_refresh_safe();
    server.send(200, "text/plain", "OK");
}

void handleSetBright() {
    if (!requireHttpAuth()) return;

    if (!server.hasArg("type") || !server.hasArg("val")) {
        server.send(400, "text/plain", "MISSING_ARG");
        return;
    }

    String type = server.arg("type");
    uint8_t value = clampByteValue(server.arg("val").toInt(), 120);

    if (type == "ports") {
        cfg.bright_ports = value;
    } else {
        server.send(400, "text/plain", "BAD_TYPE");
        return;
    }

    matrix_config_save(cfg, zoneMap, sizeof(zoneMap));
    led_refresh_safe();
    server.send(200, "text/plain", "OK");
}

void handleSaveStatusColors() {
    if (!requireHttpAuth()) return;

    String body = server.arg("plain");
    body.trim();
    if (!body.startsWith("{") || !body.endsWith("}")) {
        server.send(400, "text/plain", "BAD_JSON");
        return;
    }

    String status = extractJsonStringField(body, "status");
    String colorsJson = extractJsonObjectField(body, "colors");

    if (status.length() == 0 || !matrix_config_save_status_layer(status.c_str(), colorsJson)) {
        server.send(400, "text/plain", "BAD_STATUS");
        return;
    }

    String firstColor;
    if (findFirstHexColor(colorsJson, firstColor)) {
        if (status == "waiting") {
            parseHexColor(firstColor, cfg.color_wait);
        } else if (status == "charging") {
            parseHexColor(firstColor, cfg.color_charge);
        } else if (status == "error") {
            parseHexColor(firstColor, cfg.color_error);
        }
        matrix_config_save(cfg, zoneMap, sizeof(zoneMap));
    }

    led_reload_status_layers_safe();
    server.send(200, "text/plain", "OK");
}

void handleSaveFreeZone() {
    if (!requireHttpAuth()) return;

    String body = server.arg("plain");
    body.trim();
    if (!body.startsWith("{") || !body.endsWith("}")) {
        server.send(400, "text/plain", "BAD_JSON");
        return;
    }

    const int zoneIdValue = extractJsonIntField(body, "zoneId", -1);
    if (zoneIdValue < 0 || zoneIdValue > 255 || !zone_is_free((uint8_t)zoneIdValue)) {
        server.send(400, "text/plain", "BAD_ZONE");
        return;
    }

    const int zoneIndex = free_zone_index((uint8_t)zoneIdValue);
    if (zoneIndex < 0) {
        server.send(400, "text/plain", "BAD_ZONE");
        return;
    }

    FreeZoneConfig &freeZone = freeZoneConfigs[zoneIndex];
    freeZone.enabled = extractJsonIntField(body, "enabled", freeZone.enabled ? 1 : 0) ? 1 : 0;
    freeZone.mode = parseFreeZoneMode(extractJsonStringField(body, "mode"), freeZone.mode);
    freeZone.brightness = clampByteValue(extractJsonIntField(body, "brightness", freeZone.brightness), freeZone.brightness);

    const String staticColor = extractJsonStringField(body, "staticColor");
    if (staticColor.length() == 7) {
        parseHexColor(staticColor, freeZone.staticColor);
    }

    if (!matrix_config_save_free_zones(freeZoneConfigs, FREE_ZONE_COUNT)) {
        server.send(500, "text/plain", "SAVE_CONFIG_FAILED");
        return;
    }

    if (freeZone.mode == FREE_ZONE_CUSTOM && jsonHasField(body, "customLayer")) {
        const String customLayer = extractJsonObjectField(body, "customLayer");
        if (!matrix_config_save_free_zone_layer(freeZone.zoneId, customLayer, zoneMap, sizeof(zoneMap))) {
            server.send(400, "text/plain", "BAD_LAYER");
            return;
        }
    }

    led_reload_free_zone_layers_safe();
    led_refresh_safe();
    server.send(200, "text/plain", "OK");
}

void handleNetworkStatus() {
    if (!requireHttpAuth()) return;

    const NetworkConfig &net = network_config_get();
    String json;
    json.reserve(900);
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
    json += "\",\"apSsid\":\"";
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

    if (network_runtime_mode() == NETWORK_MODE_AP_FALLBACK) {
        WiFi.mode(WIFI_AP_STA);
    }

    const int count = WiFi.scanNetworks(false, true);
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
    WiFi.scanDelete();

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
        server.send(400, "text/plain", error);
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
    json += "\"}";
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
        server.send(400, "text/plain", error);
        return;
    }

    server.send(200, "text/plain", "OK");
}

void handleDiagnostics() {
    if (!requireHttpAuth()) return;

    usp1_inputs_update();
    status_mapper_update(usp1_inputs_get_state());

    const Usp1InputState &inputs = usp1_inputs_get_state();
    const StatusMapperState &status = status_mapper_get_state();

    String json;
    json.reserve(1400);
    json += "{\"activePortCount\":";
    json += String((int)status.activePortCount);
    json += ",\"runtimeMode\":\"runtime\"";
    json += ",\"primaryLedOutput\":{\"name\":\"LED1\",\"gpio\":";
    json += String((int)USP1_PRIMARY_LED_PIN);
    json += ",\"index\":";
    json += String((int)USP1_PRIMARY_LED_OUTPUT_INDEX + 1);
    json += "},\"ledOutputs\":[";

    for (uint8_t i = 0; i < USP1_LED_OUTPUT_COUNT; i++) {
        if (i > 0) json += ",";
        json += "{\"name\":\"LED";
        json += String((int)i + 1);
        json += "\",\"gpio\":";
        json += String((int)USP1_LED_OUTPUT_PINS[i]);
        json += ",\"state\":\"";
        json += ((i == USP1_PRIMARY_LED_OUTPUT_INDEX) ? "primary" : "reserved");
        json += "\"}";
    }

    json += "],\"inputs\":[";
    for (uint8_t i = 0; i < USP1_DATA_INPUT_COUNT; i++) {
        if (i > 0) json += ",";
        json += "{\"name\":\"Data";
        json += String((int)i + 1);
        json += "\",\"gpio\":";
        json += String((int)USP1_DATA_PINS[i]);
        json += ",\"raw\":\"";
        json += ((inputs.rawLevel[i] == LOW) ? "LOW" : "HIGH");
        json += "\",\"active\":";
        json += (inputs.active[i] ? "true" : "false");
        json += "}";
    }

    json += "],\"ports\":[";
    for (uint8_t i = 0; i < USP1_MAX_PORT_COUNT; i++) {
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

void updatePortStatusFromInputs() {
    static unsigned long lastUpdateMs = 0;

    const unsigned long now = millis();
    if (now - lastUpdateMs < 100) {
        return;
    }
    lastUpdateMs = now;

    usp1_inputs_update();
    status_mapper_update(usp1_inputs_get_state());
}

void setup() {
    Serial.begin(115200);
    auth_config_setup();
    usp1_inputs_setup();
    status_mapper_setup();
    led_setup();
    network_setup();

    Serial.println("\n=== LED matrix controller network started ===");
    Serial.print("Configurator address: http://");
    Serial.println(network_sta_connected() ? network_local_ip_string() : network_ap_ip_string());

    server.on("/", handleRoot);
    server.on("/get_size", handleGetMatrixSize);
    server.on("/get_config", handleGetConfig);
    server.on("/save_zones", HTTP_POST, handleSaveZones);
    server.on("/save_topology", HTTP_POST, handleSaveTopology);
    server.on("/set_bright", handleSetBright);
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
    updatePortStatusFromInputs();
    if (otaRestartPending && millis() >= otaRestartAt) {
        otaRestartPending = false;
        ESP.restart();
    }
}
