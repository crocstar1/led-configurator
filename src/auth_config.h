#ifndef AUTH_CONFIG_H
#define AUTH_CONFIG_H

#include <Arduino.h>

void auth_config_setup();
bool auth_config_enabled();
const char *auth_config_username();
const char *auth_config_password();
String auth_config_username_string();
bool auth_config_save_settings(
    bool enabled,
    const String &username,
    const String &password,
    String &error);
bool auth_config_clear_saved_settings();
bool auth_config_uses_default_credentials();

#endif
