// config_manager.h
#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
/*const char* ssid     = "FortBees 2.4";
const char* password = "beersoft";

const char* host     = "192.168.1.67"; // IP address of your SimHub PC
const uint16_t port  = 8008;             // SimHub Web / MJPEG Port
const char* path     = "/stream";        // MJPEG stream path
*/
struct AppConfig {
    char wifi_ssid[64]        =  "FortBees 2.4";
    char wifi_password[64]    =  "beersoft";
    char simhub_ip[32]        = "192.168.1.196";
    int  simhub_port          = 8888;
    char simhub_path[64]      = "/api/getgamedata";
    char full_simhub_url[160] = "";
};

extern AppConfig app_config;

enum ConfigStatus {
    CONFIG_LOADED,
    CONFIG_CREATED_DEFAULT,
    CONFIG_SD_ERROR
};

inline void clean_val(char* str) {
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

inline ConfigStatus load_or_create_config(const char* filepath = "/beersoft_dash.cfg") {
    if (!SD.exists(filepath)) {
        File f = SD.open(filepath, FILE_WRITE);
        if (!f) return CONFIG_SD_ERROR;

        f.println("// ================= Wi-Fi Network Credentials =================");
        f.println("WIFI_SSID     = \"YOUR_WIFI_SSID\";");
        f.println("WIFI_PASSWORD = \"YOUR_WIFI_PASSWORD\";");
        f.println("");
        f.println("// ================= SimHub Server Settings =================");
        f.println("SIMHUB_IP     = \"192.168.1.196\";");
        f.println("SIMHUB_PORT   = 8888;");
        f.println("SIMHUB_PATH   = \"/api/getgamedata\";");
        f.close();
        return CONFIG_CREATED_DEFAULT;
    }

    File f = SD.open(filepath, FILE_READ);
    if (!f) return CONFIG_SD_ERROR;

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
        }
    }
    f.close();

    snprintf(app_config.full_simhub_url, sizeof(app_config.full_simhub_url), 
             "http://%s:%d%s", app_config.simhub_ip, app_config.simhub_port, app_config.simhub_path);

    return CONFIG_LOADED;
}
