#include "auth_config.h"

#include <Preferences.h>

static constexpr const char *NVS_NAMESPACE = "auth_cfg";
static constexpr uint16_t AUTH_CONFIG_VERSION = 1;
static constexpr const char *DEFAULT_USERNAME = "admin";
static constexpr const char *DEFAULT_PASSWORD = "admin";
static constexpr size_t MAX_USERNAME_LENGTH = 32;
static constexpr size_t MIN_PASSWORD_LENGTH = 6;
static constexpr size_t MAX_PASSWORD_LENGTH = 64;

static String currentUsername = DEFAULT_USERNAME;
static String currentPassword = DEFAULT_PASSWORD;

static bool credentialsLookValid(const String &username, const String &password, bool allowDefaultPassword) {
    if (username.length() == 0 || username.length() > MAX_USERNAME_LENGTH) return false;
    if (password.length() == 0 || password.length() > MAX_PASSWORD_LENGTH) return false;
    if (!allowDefaultPassword && password.length() < MIN_PASSWORD_LENGTH) return false;
    return true;
}

static bool saveRawCredentials(const String &username, const String &password) {
    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, false)) return false;
    const size_t versionWritten = preferences.putUShort("version", AUTH_CONFIG_VERSION);
    const size_t usernameWritten = preferences.putString("username", username);
    const size_t passwordWritten = preferences.putString("password", password);
    const bool verified =
        preferences.getUShort("version", 0) == AUTH_CONFIG_VERSION &&
        preferences.getString("username", "") == username &&
        preferences.getString("password", "") == password;
    preferences.end();
    return versionWritten == sizeof(uint16_t) &&
        usernameWritten == username.length() &&
        passwordWritten == password.length() &&
        verified;
}

void auth_config_setup() {
    Preferences preferences;
    if (!preferences.begin(NVS_NAMESPACE, true)) {
        currentUsername = DEFAULT_USERNAME;
        currentPassword = DEFAULT_PASSWORD;
        return;
    }

    const uint16_t version = preferences.getUShort("version", 0);
    const String username = preferences.getString("username", "");
    const String password = preferences.getString("password", "");
    preferences.end();

    if (version != AUTH_CONFIG_VERSION || !credentialsLookValid(username, password, true)) {
        currentUsername = DEFAULT_USERNAME;
        currentPassword = DEFAULT_PASSWORD;
        if (!saveRawCredentials(currentUsername, currentPassword)) {
            Serial.println("Auth defaults could not be saved to NVS");
        }
        return;
    }

    currentUsername = username;
    currentPassword = password;
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

bool auth_config_save_credentials(const String &username, const String &password, String &error) {
    String cleanUsername = username;
    cleanUsername.trim();

    if (cleanUsername.length() == 0) {
        error = "USERNAME_REQUIRED";
        return false;
    }
    if (cleanUsername.length() > MAX_USERNAME_LENGTH) {
        error = "USERNAME_TOO_LONG";
        return false;
    }
    if (password.length() == 0) {
        error = "PASSWORD_REQUIRED";
        return false;
    }
    if (password.length() < MIN_PASSWORD_LENGTH) {
        error = "PASSWORD_TOO_SHORT";
        return false;
    }
    if (password.length() > MAX_PASSWORD_LENGTH) {
        error = "PASSWORD_TOO_LONG";
        return false;
    }

    const String previousUsername = currentUsername;
    const String previousPassword = currentPassword;
    if (!saveRawCredentials(cleanUsername, password)) {
        const bool rollbackSucceeded = saveRawCredentials(previousUsername, previousPassword);
        error = rollbackSucceeded ? "NVS_WRITE_FAILED" : "NVS_WRITE_FAILED_ROLLBACK_FAILED";
        return false;
    }

    currentUsername = cleanUsername;
    currentPassword = password;
    error = "";
    return true;
}
