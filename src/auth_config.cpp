#include "auth_config.h"

#include <Preferences.h>

static constexpr const char *NVS_NAMESPACE = "auth_cfg";
static constexpr uint16_t AUTH_CONFIG_VERSION = 1;
static constexpr const char *DEFAULT_USERNAME = "admin";
static constexpr const char *DEFAULT_PASSWORD = "admin";
static constexpr size_t MAX_USERNAME_LENGTH = 32;
static constexpr size_t MIN_PASSWORD_LENGTH = 6;
static constexpr size_t MAX_PASSWORD_LENGTH = 64;

static bool currentEnabled = false;
static String currentUsername = DEFAULT_USERNAME;
static String currentPassword = DEFAULT_PASSWORD;

static bool credentialsLookValid(const String &username, const String &password, bool allowDefaultPassword) {
    if (username.length() == 0 || username.length() > MAX_USERNAME_LENGTH) return false;
    if (password.length() == 0 || password.length() > MAX_PASSWORD_LENGTH) return false;
    if (!allowDefaultPassword && password.length() < MIN_PASSWORD_LENGTH) return false;
    return true;
}

static bool saveRawSettings(
    bool enabled,
    const String &username,
    const String &password) {
    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, false)) return false;
    const size_t versionWritten = preferences.putUShort("version", AUTH_CONFIG_VERSION);
    const size_t usernameWritten = versionWritten == sizeof(uint16_t)
        ? preferences.putString("username", username)
        : 0;
    const size_t passwordWritten = usernameWritten == username.length()
        ? preferences.putString("password", password)
        : 0;
    // Write the access-control flag last so an interrupted first-time enable
    // does not expose partially written credentials as an active login.
    const size_t enabledWritten = passwordWritten == password.length()
        ? preferences.putBool("enabled", enabled)
        : 0;
    const bool verified =
        preferences.getUShort("version", 0) == AUTH_CONFIG_VERSION &&
        preferences.getString("username", "") == username &&
        preferences.getString("password", "") == password &&
        preferences.getBool("enabled", !enabled) == enabled;
    preferences.end();
    return versionWritten == sizeof(uint16_t) &&
        usernameWritten == username.length() &&
        passwordWritten == password.length() &&
        enabledWritten == sizeof(uint8_t) && verified;
}

void auth_config_setup() {
    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, true)) {
        currentEnabled = false;
        currentUsername = DEFAULT_USERNAME;
        currentPassword = DEFAULT_PASSWORD;
        return;
    }

    const uint16_t version = preferences.getUShort("version", 0);
    const bool enabled = preferences.getBool("enabled", false);
    const String username = preferences.getString("username", "");
    const String password = preferences.getString("password", "");
    preferences.end();

    if (version != AUTH_CONFIG_VERSION || !credentialsLookValid(username, password, true)) {
        currentEnabled = false;
        currentUsername = DEFAULT_USERNAME;
        currentPassword = DEFAULT_PASSWORD;
        if (!saveRawSettings(currentEnabled, currentUsername, currentPassword)) {
            Serial.println("Auth defaults could not be saved to NVS");
        }
        return;
    }

    currentEnabled = enabled;
    currentUsername = username;
    currentPassword = password;
}

bool auth_config_enabled() {
    return currentEnabled;
}

const char *auth_config_username() {
    return currentUsername.c_str();
}

const char *auth_config_password() {
    return currentPassword.c_str();
}

String auth_config_username_string() {
    return currentUsername;
}

bool auth_config_uses_default_credentials() {
    return currentUsername == DEFAULT_USERNAME && currentPassword == DEFAULT_PASSWORD;
}

bool auth_config_save_settings(
    bool enabled,
    const String &username,
    const String &password,
    String &error) {
    String cleanUsername = currentUsername;
    String nextPassword = currentPassword;
    if (enabled) {
        cleanUsername = username;
        cleanUsername.trim();
        nextPassword = password;

        if (cleanUsername.length() == 0) {
            error = "USERNAME_REQUIRED";
            return false;
        }
        if (cleanUsername.length() > MAX_USERNAME_LENGTH) {
            error = "USERNAME_TOO_LONG";
            return false;
        }
        if (nextPassword.length() == 0) {
            error = "PASSWORD_REQUIRED";
            return false;
        }
        if (nextPassword.length() < MIN_PASSWORD_LENGTH) {
            error = "PASSWORD_TOO_SHORT";
            return false;
        }
        if (nextPassword.length() > MAX_PASSWORD_LENGTH) {
            error = "PASSWORD_TOO_LONG";
            return false;
        }
    }

    const bool previousEnabled = currentEnabled;
    const String previousUsername = currentUsername;
    const String previousPassword = currentPassword;
    if (!saveRawSettings(enabled, cleanUsername, nextPassword)) {
        const bool rollbackSucceeded = saveRawSettings(
            previousEnabled, previousUsername, previousPassword);
        error = rollbackSucceeded ? "NVS_WRITE_FAILED" : "NVS_WRITE_FAILED_ROLLBACK_FAILED";
        return false;
    }

    currentEnabled = enabled;
    currentUsername = cleanUsername;
    currentPassword = nextPassword;
    error = "";
    return true;
}

bool auth_config_clear_saved_settings() {
    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, false)) return false;

    const bool cleared = preferences.clear();
    preferences.end();
    return cleared;
}
