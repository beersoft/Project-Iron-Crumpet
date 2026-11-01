#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

struct WheelData {
    float temp_c   = 0.0f;
    float brake_c  = 0.0f;
    float pressure = 0.0f;
};

struct SectorInfo {
    char time_str[12] = {0};
    uint8_t status    = 0; // 0 = None, 1 = Yellow, 2 = Green, 3 = Purple
};

struct SimTelemetry {
    bool connected      = false;
    bool game_running   = false;
    bool in_race        = false;
    bool game_paused    = false;
    bool game_replay    = false;
    char game_name[32]  = {0};
    char track_name[32] = {0};

    int position              = 0;
    int current_lap           = 0;
    int total_laps            = 0;
    char current_lap_time[16] = {0};
    char last_lap_time[24]    = {0};
    char best_lap_time[24]    = {0};
    char all_time_best[24]    = {0};

    // Lap Deltas & Splits
    float delta_to_best                     = 0.0f; // DeltaToSessionBest
    float session_best_live_delta           = 0.0f; // SessionBestLiveDeltaProgressSeconds
    float lap_delta_vs_prev_best            = 0.0f; // LapDeltaVsPreviousSessionBest
    float self_split_delta                  = 0.0f; // SelfsplitDelta
    bool  delta_valid                       = false;

    // Sector Details Array
    SectorInfo sectors[3];

    char sector_1[16]       = {0};
    char sector_2[16]       = {0};
    char sector_3[16]       = {0};
    uint8_t sector_1_status = 0;
    uint8_t sector_2_status = 0;
    uint8_t sector_3_status = 0;

    float speed_kmh = 0.0f;
    int rpm         = 0;
    int max_rpm     = 8000;
    char gear[4]    = "N";

    bool abs_available         = false;
    int abs_level              = 0;
    bool abs_active            = false;

    bool tc_available          = false;
    int tc_level               = 0;
    bool tc_active             = false;

    bool pit_limiter_available = false;
    bool pit_limiter_on        = false;

    bool lights_available      = false;
    bool headlights_on         = false;
    bool highbeam_flash_on     = false;

    bool wipers_available      = false;
    bool wipers_on             = false;

    bool ignition_available    = false;
    bool engine_running        = false;

    int tc_cut                 = 0;
    int tc_slip                = 0;
    int engine_map             = 0;
    float brake_bias           = 0.0f;
    float brake_migration      = 0.0f;
    bool handbrake_active      = false;
    char flag_name[16]         = "None";

    float fuel_pct             = 0.0f;
    float battery_charge_pct   = 0.0f;
    float air_temperature      = 0.0f;

    bool spotter_left          = false;
    bool spotter_right         = false;

    WheelData fl;
    WheelData fr;
    WheelData rl;
    WheelData rr;
};

extern SimTelemetry live_sim_data;
extern SemaphoreHandle_t sim_data_mutex;
extern AppConfig app_config;

inline void format_simhub_time_string(char* dest, size_t dest_len, const char* src) {
    if (!src || strlen(src) == 0 || strcmp(src, "00:00:00") == 0) {
        snprintf(dest, dest_len, "--:--.---");
        return;
    }
    if (strlen(src) >= 12 && src[2] == ':') {
        const char* p = src;
        if (strncmp(p, "00:", 3) == 0) {
            p += 3;
        }
        strncpy(dest, p, dest_len - 1);
        dest[dest_len - 1] = '\0';
        if (strlen(dest) > 9 && dest[5] == '.') {
            dest[9] = '\0';
        }
        return;
    }
    strncpy(dest, src, dest_len - 1);
    dest[dest_len - 1] = '\0';
}

inline void parse_simhub_payload(JsonDocument& root) {
    if (sim_data_mutex == NULL) return;

    if (xSemaphoreTake(sim_data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        live_sim_data.connected = true;

        if (root.containsKey("GameName")) {
            const char* gname = root["GameName"] | "AssettoCorsa";
            strncpy(live_sim_data.game_name, gname, sizeof(live_sim_data.game_name) - 1);
        }
        live_sim_data.game_running = root["GameRunning"] | true;
        live_sim_data.in_race      = root["IsGameInRace"] | root["InRace"] | true;
        live_sim_data.game_paused  = root["GamePaused"] | false;
        live_sim_data.game_replay  = root["GameReplay"] | false;

        JsonObject d = root.containsKey("NewData") ? root["NewData"].as<JsonObject>() : root.as<JsonObject>();

        // Track Name Parser
        if (d.containsKey("TrackCode") || d.containsKey("TrackName") || d.containsKey("TrackId") || d.containsKey("Track")) {
            const char* t = d["TrackCode"] | d["TrackName"] | d["TrackId"] | d["Track"] | "";
            strncpy(live_sim_data.track_name, t, sizeof(live_sim_data.track_name) - 1);
        } else if (root.containsKey("TrackCode") || root.containsKey("TrackName")) {
            const char* t = root["TrackCode"] | root["TrackName"] | "";
            strncpy(live_sim_data.track_name, t, sizeof(live_sim_data.track_name) - 1);
        }

        live_sim_data.speed_kmh = d["SpeedKmh"] | d["SpeedLocal"] | d["FilteredSpeedKmh"] | 0.0f;
        live_sim_data.rpm       = (int)(d["Rpms"] | d["RPM"] | d["FilteredRpms"] | 0.0f);
        live_sim_data.max_rpm   = (int)(d["MaxRpm"] | d["MaxRPM"] | 6800.0f);

        if (d.containsKey("Gear")) {
            if (d["Gear"].is<const char*>()) {
                const char* g = d["Gear"].as<const char*>();
                strncpy(live_sim_data.gear, g ? g : "N", sizeof(live_sim_data.gear) - 1);
            } else {
                int g_num = d["Gear"].as<int>();
                if (g_num == 0) strncpy(live_sim_data.gear, "N", sizeof(live_sim_data.gear) - 1);
                else if (g_num < 0) strncpy(live_sim_data.gear, "R", sizeof(live_sim_data.gear) - 1);
                else snprintf(live_sim_data.gear, sizeof(live_sim_data.gear), "%d", g_num);
            }
        }

        if (d.containsKey("ABSActive") || d.containsKey("ABSLevel")) {
            live_sim_data.abs_level     = d["ABSLevel"] | 0;
            live_sim_data.abs_available = (live_sim_data.abs_level > 0) || d.containsKey("ABSActive");
            live_sim_data.abs_active    = (d["ABSActive"] | 0) > 0;
        } else {
            live_sim_data.abs_available = false;
            live_sim_data.abs_active    = false;
            live_sim_data.abs_level     = 0;
        }

        if (d.containsKey("TCActive") || d.containsKey("TCLevel")) {
            live_sim_data.tc_level     = d["TCLevel"] | 0;
            live_sim_data.tc_available = (live_sim_data.tc_level > 0) || d.containsKey("TCActive");
            live_sim_data.tc_active    = (d["TCActive"] | 0) > 0;
        } else {
            live_sim_data.tc_available = false;
            live_sim_data.tc_active    = false;
            live_sim_data.tc_level     = 0;
        }

        live_sim_data.pit_limiter_available = d.containsKey("PitLimiterOn") || d.containsKey("PitLimiter");
        live_sim_data.pit_limiter_on        = (d["PitLimiterOn"] | d["PitLimiter"] | 0) > 0;

        // Headlamps & Highbeams
        live_sim_data.lights_available  = d.containsKey("HeadlightsOn") || d.containsKey("Headlights") || d.containsKey("Highbeams") || d.containsKey("FlashLights");
        live_sim_data.headlights_on     = (d["HeadlightsOn"] | d["Headlights"] | 0) > 0;
        live_sim_data.highbeam_flash_on = (d["Highbeams"] | d["FlashLights"] | 0) > 0;

        live_sim_data.wipers_available  = d.containsKey("WipersOn") || d.containsKey("Wipers");
        live_sim_data.wipers_on         = (d["WipersOn"] | d["Wipers"] | 0) > 0;

        live_sim_data.ignition_available = d.containsKey("EngineStarted") || d.containsKey("EngineIgnitionOn");
        int eng_started = d["EngineStarted"] | 0;
        int eng_ign     = d["EngineIgnitionOn"] | 0;
        live_sim_data.engine_running    = (eng_started == 1 || eng_ign == 1 || live_sim_data.rpm > 350);

        live_sim_data.position    = d["Position"] | d["PlayerLeaderboardPosition"] | 1;
        live_sim_data.current_lap = d["CurrentLap"] | 1;
        live_sim_data.total_laps  = d["TotalLaps"] | 0;

        format_simhub_time_string(live_sim_data.current_lap_time, sizeof(live_sim_data.current_lap_time), d["CurrentLapTime"] | "");
        format_simhub_time_string(live_sim_data.last_lap_time,    sizeof(live_sim_data.last_lap_time),    d["LastLapTime"] | "");
        format_simhub_time_string(live_sim_data.best_lap_time,    sizeof(live_sim_data.best_lap_time),    d["BestLapTime"] | "");
        format_simhub_time_string(live_sim_data.all_time_best,    sizeof(live_sim_data.all_time_best),    d["AllTimeBest"] | "");

        // Lap Deltas & Splits Parsing
        live_sim_data.delta_to_best           = d["DeltaToSessionBest"] | 0.0f;
        live_sim_data.session_best_live_delta = d["SessionBestLiveDeltaProgressSeconds"] | 0.0f;
        live_sim_data.lap_delta_vs_prev_best  = d["LapDeltaVsPreviousSessionBest"] | 0.0f;
        live_sim_data.self_split_delta        = d["SelfsplitDelta"] | 0.0f;
        live_sim_data.delta_valid             = !d["DeltaToSessionBest"].isNull() || !d["SessionBestLiveDeltaProgressSeconds"].isNull() || !d["LapDeltaVsPreviousSessionBest"].isNull();

        // Sector Details Array Parsing
        if (d.containsKey("SectorDetails") && d["SectorDetails"].is<JsonArray>()) {
            JsonArray sector_array = d["SectorDetails"].as<JsonArray>();
            int idx = 0;
            for (JsonObject sec : sector_array) {
                if (idx < 3) {
                    const char* stime = sec["SectorTime"] | sec["Time"] | "";
                    format_simhub_time_string(live_sim_data.sectors[idx].time_str, sizeof(live_sim_data.sectors[idx].time_str), stime);
                    live_sim_data.sectors[idx].status = sec["SectorStatus"] | sec["Status"] | 0;
                    idx++;
                }
            }
        }

        live_sim_data.brake_bias     = d["BrakeBias"] | 0.0f;
        live_sim_data.engine_map     = d["EngineMap"] | 0;

        live_sim_data.spotter_left  = (d["SpotterCarLeft"] | 0) > 0;
        live_sim_data.spotter_right = (d["SpotterCarRight"] | 0) > 0;

        live_sim_data.fuel_pct        = d["FuelPercent"] | 0.0f;
        live_sim_data.air_temperature = d["AirTemperature"] | 0.0f;

        live_sim_data.fl.temp_c   = d["TyreTemperatureFrontLeft"] | 0.0f;
        live_sim_data.fr.temp_c   = d["TyreTemperatureFrontRight"] | 0.0f;
        live_sim_data.rl.temp_c   = d["TyreTemperatureRearLeft"] | 0.0f;
        live_sim_data.rr.temp_c   = d["TyreTemperatureRearRight"] | 0.0f;

        live_sim_data.fl.pressure = d["TyrePressureFrontLeft"] | 0.0f;
        live_sim_data.fr.pressure = d["TyrePressureFrontRight"] | 0.0f;
        live_sim_data.rl.pressure = d["TyrePressureRearLeft"] | 0.0f;
        live_sim_data.rr.pressure = d["TyrePressureRearRight"] | 0.0f;

        live_sim_data.fl.brake_c  = d["BrakeTemperatureFrontLeft"] | 0.0f;
        live_sim_data.fr.brake_c  = d["BrakeTemperatureFrontRight"] | 0.0f;
        live_sim_data.rl.brake_c  = d["BrakeTemperatureRearLeft"] | 0.0f;
        live_sim_data.rr.brake_c  = d["BrakeTemperatureRearRight"] | 0.0f;

        xSemaphoreGive(sim_data_mutex);
    }
}

inline void simhub_poll_task(void* pvParameters) {
    HTTPClient http;
    http.setReuse(true);
    http.setTimeout(1500);

    DynamicJsonDocument doc(16384);
    char active_url[128] = {0};
    bool echo_needed = true;

    if (strlen(app_config.simhub_ip) > 0 && strcmp(app_config.simhub_ip, "0.0.0.0") != 0) {
        snprintf(active_url, sizeof(active_url), "http://%s:%d/api/getgamedata",
                 app_config.simhub_ip, app_config.simhub_port);
    }

    while (true) {
        if (WiFi.status() == WL_CONNECTED) {
            if (strlen(active_url) == 0) {
                IPAddress lip = WiFi.localIP();
                char test_url[96];
                for (int host = 1; host < 255; host++) {
                    if (host == lip[3]) continue;
                    snprintf(test_url, sizeof(test_url), "http://%d.%d.%d.%d:%d/api/getgamedata",
                             lip[0], lip[1], lip[2], host, app_config.simhub_port);
                    
                    http.begin(test_url);
                    http.setTimeout(300);
                    int testCode = http.GET();
                    if (testCode == HTTP_CODE_OK) {
                        strncpy(active_url, test_url, sizeof(active_url) - 1);
                        http.end();
                        Serial.printf("[AUTODETECT] Connected to SimHub at: %s\n", active_url);
                        echo_needed = true;
                        break;
                    }
                    http.end();
                    vTaskDelay(pdMS_TO_TICKS(2));
                }
                if (strlen(active_url) == 0) {
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    continue;
                }
            }

            http.begin(active_url);
            http.addHeader("Connection", "keep-alive");
            http.setTimeout(1500);
            
            int httpCode = http.GET();

            if (httpCode == HTTP_CODE_OK) {
                String payload = http.getString();

                if (echo_needed) {
                    Serial.println("\n================ INCOMING SIMHUB JSON ================");
                    Serial.printf("Payload Length: %u bytes\n", payload.length());
                    Serial.println(payload);
                    Serial.println("======================================================\n");
                    echo_needed = false;
                }

                if (payload.length() > 50 && payload.endsWith("}")) {
                    doc.clear();
                    DeserializationError err = deserializeJson(doc, payload);

                    if (!err) {
                        parse_simhub_payload(doc);
                    } else {
                        Serial.printf("[JSON ERR] %s (len: %d)\n", err.c_str(), payload.length());
                    }
                }
            } else {
                if (xSemaphoreTake(sim_data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                    live_sim_data.connected = false;
                    xSemaphoreGive(sim_data_mutex);
                }
                echo_needed = true;
                
                if (httpCode <= 0) {
                    Serial.printf("[HTTP] SimHub timeout (%d). Retrying...\n", httpCode);
                    active_url[0] = '\0';
                }
            }
            http.end();
        } else {
            if (xSemaphoreTake(sim_data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                live_sim_data.connected = false;
                xSemaphoreGive(sim_data_mutex);
            }
            echo_needed = true;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}