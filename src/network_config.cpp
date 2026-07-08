#include "network_config.h"

#include <Preferences.h>

static constexpr const char *NVS_NAMESPACE = "net_cfg";
static constexpr const char *DEFAULT_AP_SSID = "LED_MATRIX_SETUP";
static constexpr const char *DEFAULT_AP_PASSWORD = "12345678";
static constexpr unsigned long STA_CONNECT_TIMEOUT_MS = 12000;

static NetworkConfig networkConfig;
static NetworkRuntimeMode runtimeMode = NETWORK_MODE_AP_FALLBACK;
static String connectionStatus = "not_configured";
static unsigned long pendingApplyAt = 0;

static IPAddress defaultStaticIp(192, 168, 1, 150);
static IPAddress defaultGateway(192, 168, 1, 1);
static IPAddress defaultSubnet(255, 255, 255, 0);
static IPAddress defaultDns(8, 8, 8, 8);
static IPAddress defaultApIp(192, 168, 4, 1);
static IPAddress defaultApSubnet(255, 255, 255, 0);

String network_ip_to_string(const IPAddress &ip) {
    return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

bool network_parse_ip(const String &value, IPAddress &ip) {
    String trimmed = value;
    trimmed.trim();
    return trimmed.length() > 0 && ip.fromString(trimmed);
}

static String loadString(Preferences &preferences, const char *key, const String &fallback) {
    return preferences.getString(key, fallback);
}

static void setDefaults(NetworkConfig &config) {
    config.ssid = "";
    config.password = "";
    config.dhcp = true;
    config.staticIp = defaultStaticIp;
    config.gateway = defaultGateway;
    config.subnet = defaultSubnet;
    config.dns = defaultDns;
    config.apSsid = DEFAULT_AP_SSID;
    config.apPassword = DEFAULT_AP_PASSWORD;
}

static bool validateConfig(const NetworkConfig &config, String &error) {
    if (config.ssid.length() > 32) {
        error = "SSID_TOO_LONG";
        return false;
    }
    if (config.password.length() > 64) {
        error = "PASSWORD_TOO_LONG";
        return false;
    }
    if (config.apSsid.length() == 0 || config.apSsid.length() > 32) {
        error = "BAD_AP_SSID";
        return false;
    }
    if (config.apPassword.length() < 8 || config.apPassword.length() > 64) {
        error = "BAD_AP_PASSWORD";
        return false;
    }
    if (!config.dhcp) {
        if (config.staticIp == IPAddress(0, 0, 0, 0) ||
            config.subnet == IPAddress(0, 0, 0, 0) ||
            config.gateway == IPAddress(0, 0, 0, 0) ||
            config.dns == IPAddress(0, 0, 0, 0)) {
            error = "BAD_STATIC_IP";
            return false;
        }
    }
    return true;
}

static void loadConfig() {
    setDefaults(networkConfig);

    Preferences preferences;
    preferences.begin(NVS_NAMESPACE, true);
    networkConfig.ssid = loadString(preferences, "ssid", "");
    networkConfig.password = loadString(preferences, "pass", "");
    networkConfig.dhcp = preferences.getBool("dhcp", true);
    networkConfig.staticIp.fromString(loadString(preferences, "ip", network_ip_to_string(defaultStaticIp)));
    networkConfig.gateway.fromString(loadString(preferences, "gw", network_ip_to_string(defaultGateway)));
    networkConfig.subnet.fromString(loadString(preferences, "mask", network_ip_to_string(defaultSubnet)));
    networkConfig.dns.fromString(loadString(preferences, "dns", network_ip_to_string(defaultDns)));
    networkConfig.apSsid = loadString(preferences, "ap_ssid", DEFAULT_AP_SSID);
    networkConfig.apPassword = loadString(preferences, "ap_pass", DEFAULT_AP_PASSWORD);
    preferences.end();

    String ignored;
    if (!validateConfig(networkConfig, ignored)) {
        setDefaults(networkConfig);
    }
}

static void startApFallback(const char *reason) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(defaultApIp, defaultApIp, defaultApSubnet);
    WiFi.softAP(networkConfig.apSsid.c_str(), networkConfig.apPassword.c_str());
    runtimeMode = NETWORK_MODE_AP_FALLBACK;
    connectionStatus = reason;
    Serial.print("AP fallback: ");
    Serial.print(networkConfig.apSsid);
    Serial.print(" / ");
    Serial.println(WiFi.softAPIP());
}

static bool startSta() {
    runtimeMode = NETWORK_MODE_CONNECTING;
    connectionStatus = "connecting";

    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setHostname("led-controller");

    if (!networkConfig.dhcp) {
        WiFi.config(networkConfig.staticIp, networkConfig.gateway, networkConfig.subnet, networkConfig.dns);
    }

    WiFi.begin(networkConfig.ssid.c_str(), networkConfig.password.c_str());

    const unsigned long startedAt = millis();
    while (millis() - startedAt < STA_CONNECT_TIMEOUT_MS) {
        if (WiFi.status() == WL_CONNECTED) {
            runtimeMode = NETWORK_MODE_STA;
            connectionStatus = "connected";
            Serial.print("WiFi STA connected: ");
            Serial.print(WiFi.SSID());
            Serial.print(" / ");
            Serial.println(WiFi.localIP());
            return true;
        }
        delay(250);
    }

    connectionStatus = "connect_timeout";
    return false;
}

static void applyConfig() {
    if (networkConfig.ssid.length() == 0) {
        startApFallback("not_configured");
        return;
    }

    if (!startSta()) {
        startApFallback("connect_failed");
    }
}

void network_setup() {
    loadConfig();
    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
    applyConfig();
}

void network_loop() {
    if (pendingApplyAt > 0 && millis() >= pendingApplyAt) {
        pendingApplyAt = 0;
        applyConfig();
    }
}

const NetworkConfig &network_config_get() {
    return networkConfig;
}

bool network_config_save(const NetworkConfig &config, String &error) {
    if (!validateConfig(config, error)) {
        return false;
    }

    Preferences preferences;
    preferences.begin(NVS_NAMESPACE, false);
    preferences.putString("ssid", config.ssid);
    preferences.putString("pass", config.password);
    preferences.putBool("dhcp", config.dhcp);
    preferences.putString("ip", network_ip_to_string(config.staticIp));
    preferences.putString("gw", network_ip_to_string(config.gateway));
    preferences.putString("mask", network_ip_to_string(config.subnet));
    preferences.putString("dns", network_ip_to_string(config.dns));
    preferences.putString("ap_ssid", config.apSsid);
    preferences.putString("ap_pass", config.apPassword);
    preferences.end();

    networkConfig = config;
    return true;
}

void network_schedule_apply(unsigned long delayMs) {
    pendingApplyAt = millis() + delayMs;
}

NetworkRuntimeMode network_runtime_mode() {
    return runtimeMode;
}

const char *network_runtime_mode_string() {
    switch (runtimeMode) {
        case NETWORK_MODE_STA: return "sta";
        case NETWORK_MODE_CONNECTING: return "connecting";
        case NETWORK_MODE_AP_FALLBACK:
        default: return "ap_fallback";
    }
}

const char *network_connection_status_string() {
    return connectionStatus.c_str();
}

bool network_has_saved_ssid() {
    return networkConfig.ssid.length() > 0;
}

bool network_sta_connected() {
    return WiFi.status() == WL_CONNECTED && runtimeMode == NETWORK_MODE_STA;
}

String network_connected_ssid() {
    return network_sta_connected() ? WiFi.SSID() : "";
}

String network_local_ip_string() {
    return network_sta_connected() ? network_ip_to_string(WiFi.localIP()) : "";
}

String network_ap_ip_string() {
    return runtimeMode == NETWORK_MODE_AP_FALLBACK ? network_ip_to_string(WiFi.softAPIP()) : "";
}

int32_t network_rssi() {
    return network_sta_connected() ? WiFi.RSSI() : 0;
}

int network_ap_client_count() {
    return runtimeMode == NETWORK_MODE_AP_FALLBACK ? WiFi.softAPgetStationNum() : 0;
}
