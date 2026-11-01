#include "config_manager.h"
#include <Preferences.h>
#include <FS.h>
#include <SD.h>

const char* game_override_names[] = {
    "AUTO", "Assetto Corsa", "iRacing", "Automobilista 2", "Le Mans Ultimate", "F1 24", "EA Sports WRC"
};
const size_t NUM_GAME_OVERRIDES = sizeof(game_override_names) / sizeof(game_override_names[0]);

const char* class_override_names[] = {
    "AUTO", "GT3", "GT4", "F1", "Hypercar", "Cup", "TCR", "Road"
};
const size_t NUM_CLASS_OVERRIDES = sizeof(class_override_names) / sizeof(class_override_names[0]);

// Global configuration instance definition
AppConfig app_config;

// -----------------------------------------------------------------------------
// String Cleaning Helper
// Strips quotes, semicolons, tabs, and trailing whitespace from cfg values
// -----------------------------------------------------------------------------
static void clean_val(char* str) {
    char* p = str;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '"') p++;

    int len = strlen(p);
    while (len > 0 && (p[len - 1] == ' ' || p[len - 1] == '\t' || 
                       p[len - 1] == '\r' || p[len - 1] == '\n' || 
                       p[len - 1] == ';' || p[len - 1] == '"')) {
        p[--len] = '\0';
    }
    memmove(str, p, len + 1);
}

// -----------------------------------------------------------------------------
// Save Settings to Internal NVS Flash
// -----------------------------------------------------------------------------
bool save_config_to_nvs() {
    Preferences prefs;
    if (!prefs.begin("beersoft", false)) return false;

    prefs.putString("wifi_ssid", app_config.wifi_ssid);
    prefs.putString("wifi_pass", app_config.wifi_password);
    prefs.putString("simhub_ip", app_config.simhub_ip);
    prefs.putInt("simhub_port", app_config.simhub_port);
    prefs.putString("simhub_path", app_config.simhub_path);

    // Save UI & Hardware settings
    prefs.putBool("use_mph", app_config.use_mph);
    prefs.putUChar("bright_idx", app_config.brightness_idx);
    prefs.putUChar("sleep_idx", app_config.idle_timeout_idx);
    prefs.putUChar("def_page", app_config.default_page);

    // Save Overrides
    prefs.putUChar("game_ovr", app_config.game_override_idx);
    prefs.putUChar("class_ovr", app_config.class_override_idx);

    prefs.putBool("setup_done", true);
    prefs.end();

    snprintf(app_config.full_simhub_url, sizeof(app_config.full_simhub_url), 
             "http://%s:%d%s", app_config.simhub_ip, app_config.simhub_port, app_config.simhub_path);
    return true;
}

// -----------------------------------------------------------------------------
// Load Configuration: Checks SD Card first, falls back to NVS Flash
// -----------------------------------------------------------------------------
ConfigStatus load_or_create_config(const char* filepath) {
    // 1. Try reading network overrides from SD Card if present
    if (SD.exists(filepath)) {
        File f = SD.open(filepath, FILE_READ);
        if (f) {
            while (f.available()) {
                String line = f.readStringUntil('\n');
                line.trim();

                if (line.length() == 0 || line.startsWith("//") || line.startsWith("#")) continue;

                int eqIdx = line.indexOf('=');
                if (eqIdx == -1) continue;

                String key = line.substring(0, eqIdx);
                String val = line.substring(eqIdx + 1);
                key.trim();
                val.trim();

                char buffer[128];
                val.toCharArray(buffer, sizeof(buffer));
                clean_val(buffer);

                if (key.equalsIgnoreCase("WIFI_SSID")) {
                    strncpy(app_config.wifi_ssid, buffer, sizeof(app_config.wifi_ssid) - 1);
                } else if (key.equalsIgnoreCase("WIFI_PASSWORD")) {
                    strncpy(app_config.wifi_password, buffer, sizeof(app_config.wifi_password) - 1);
                } else if (key.equalsIgnoreCase("SIMHUB_IP")) {
                    strncpy(app_config.simhub_ip, buffer, sizeof(app_config.simhub_ip) - 1);
                } else if (key.equalsIgnoreCase("SIMHUB_PORT")) {
                    app_config.simhub_port = atoi(buffer);
                } else if (key.equalsIgnoreCase("SIMHUB_PATH")) {
                    strncpy(app_config.simhub_path, buffer, sizeof(app_config.simhub_path) - 1);
                } else if (key.equalsIgnoreCase("SPEED_UNIT")) {
                    app_config.use_mph = (strcasecmp(buffer, "MPH") == 0);
                }
            }
            f.close();

            // Sync SD card overrides to internal flash
            save_config_to_nvs();
            return CONFIG_LOADED_FROM_SD;
        }
    } else {
        File f = SD.open(filepath, FILE_WRITE);
        if (f) {
            f.println("// ================= Wi-Fi Network Credentials =================");
            f.println("WIFI_SSID     = \"FortBees 2.4\";");
            f.println("WIFI_PASSWORD = \"beersoft\";");
            f.println("");
            f.println("// ================= SimHub Server Settings =================");
            f.println("SIMHUB_IP     = \"192.168.1.196\";");
            f.println("SIMHUB_PORT   = 8888;");
            f.println("SIMHUB_PATH   = \"/api/getgamedata\";");
            f.println("");
            f.println("// ================= Dashboard Preferences =================");
            f.println("SPEED_UNIT    = \"KPH\"; // KPH or MPH");
            f.close();
        }
    }

    // 2. Fallback: Load persistent settings from internal NVS Flash
    Preferences prefs;
    if (prefs.begin("beersoft", true)) { // Read-only mode
        if (prefs.isKey("setup_done")) {
            String ssid = prefs.getString("wifi_ssid", app_config.wifi_ssid);
            String pass = prefs.getString("wifi_pass", app_config.wifi_password);
            String ip   = prefs.getString("simhub_ip", app_config.simhub_ip);
            int port    = prefs.getInt("simhub_port", app_config.simhub_port);
            String path = prefs.getString("simhub_path", app_config.simhub_path);

            strncpy(app_config.wifi_ssid, ssid.c_str(), sizeof(app_config.wifi_ssid) - 1);
            strncpy(app_config.wifi_password, pass.c_str(), sizeof(app_config.wifi_password) - 1);
            strncpy(app_config.simhub_ip, ip.c_str(), sizeof(app_config.simhub_ip) - 1);
            app_config.simhub_port = port;
            strncpy(app_config.simhub_path, path.c_str(), sizeof(app_config.simhub_path) - 1);

            app_config.use_mph            = prefs.getBool("use_mph", app_config.use_mph);
            app_config.brightness_idx     = prefs.getUChar("bright_idx", app_config.brightness_idx);
            app_config.idle_timeout_idx   = prefs.getUChar("sleep_idx", app_config.idle_timeout_idx);
            app_config.default_page       = prefs.getUChar("def_page", app_config.default_page);
            app_config.game_override_idx  = prefs.getUChar("game_ovr", 0);
            app_config.class_override_idx = prefs.getUChar("class_ovr", 0);
        }
        prefs.end();
    }

    snprintf(app_config.full_simhub_url, sizeof(app_config.full_simhub_url), 
             "http://%s:%d%s", app_config.simhub_ip, app_config.simhub_port, app_config.simhub_path);

    return CONFIG_LOADED_FROM_NVS;
}