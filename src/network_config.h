#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <Arduino.h>
#include <WiFi.h>

struct NetworkConfig {
    String ssid;
    String password;
    bool dhcp;
    IPAddress staticIp;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns;
    String apSsid;
    String apPassword;
};

enum NetworkRuntimeMode {
    NETWORK_MODE_STA,
    NETWORK_MODE_AP_FALLBACK,
    NETWORK_MODE_CONNECTING,
    NETWORK_MODE_RECONNECTING
};

void network_setup();
void network_loop();
const NetworkConfig &network_config_get();
bool network_config_save(const NetworkConfig &config, String &error);
void network_schedule_apply(unsigned long delayMs);

NetworkRuntimeMode network_runtime_mode();
const char *network_runtime_mode_string();
const char *network_connection_status_string();
bool network_has_saved_ssid();
bool network_sta_connected();
String network_connected_ssid();
String network_local_ip_string();
String network_ap_ip_string();
int32_t network_rssi();
int network_ap_client_count();
String network_ip_to_string(const IPAddress &ip);
bool network_parse_ip(const String &value, IPAddress &ip);

#endif
