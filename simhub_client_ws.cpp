#include "simhub_client_ws.h"
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#include "config_manager.h"
#include "Iron-Crumpet-Template.h"

// Define the global telemetry instance and thread-safety mutex
SimTelemetry live_sim_data;
SemaphoreHandle_t sim_data_mutex = xSemaphoreCreateMutex();

namespace ActiveTemplate = DashTemplate_IronCrumpet;

// -----------------------------------------------------------------------------
// SimTelemetry Method Implementations
// -----------------------------------------------------------------------------
void SimTelemetry::reset_to_zero() {
    connected      = false;
    game_running   = false;
    in_race        = false;
    game_paused    = false;
    game_replay    = false;
    requested_page = -1;

    strncpy(game_name,  "NO SIM RUNNING",   sizeof(game_name));
    strncpy(track_name, "",                 sizeof(track_name));
    strncpy(car_name,   "NO CAR DETECTED",  sizeof(car_name));
    strncpy(car_class,  "SHITBOX",          sizeof(car_class));

    speed_kmh      = 0.0f;
    rpm            = 0;
    max_rpm        = 8000;
    turbo_boost    = 0.0f;
    strncpy(gear, "N", sizeof(gear));

    position       = 0;
    current_lap    = 0;
    total_laps     = 0;

    strncpy(current_lap_time, "--:--:--.-", sizeof(current_lap_time));
    strncpy(last_lap_time,    "--:--:--.-", sizeof(last_lap_time));
    strncpy(best_lap_time,    "--:--:--.-", sizeof(best_lap_time));
    strncpy(all_time_best,    "--:--:--.-", sizeof(all_time_best));

    delta_to_best           = 0.0f;
    session_best_live_delta = 0.0f;
    lap_delta_vs_prev_best  = 0.0f;
    self_split_delta        = 0.0f;
    delta_valid             = false;

    abs_available  = false;
    abs_active     = false;
    abs_level      = 0;

    tc_available   = false;
    tc_active      = false;
    tc_level       = 0;
    tc_cut         = 0;
    tc_slip        = 0;

    pit_limiter_available = false;
    pit_limiter_on        = false;

    lights_available   = false;
    headlights_on      = false;
    highbeam_flash_on  = false;

    wipers_available   = false;
    wipers_on          = false;

    ignition_available = false;
    engine_running     = false;

    engine_map         = 0;
    brake_bias         = 0.0f;
    brake_migration    = 0.0f;
    handbrake_active   = false;
    strncpy(flag_name, "None", sizeof(flag_name));

    fuel_pct           = 0.0f;
    battery_charge_pct = 0.0f;
    air_temperature    = 0.0f;
    water_temperature  = 0.0f;
    oil_temperature    = 0.0f;
    oil_pressure       = 0.0f;
    fuel_pressure      = 0.0f;
    road_temperature   = 0.0f;
    strncpy(session_type, "Race", sizeof(session_type));

    turn_indicator_left   = false;
    turn_indicator_right  = false;
    spotter_left          = false;
    spotter_right         = false;

    opponent_behind_dist   = 12.0f;
    opponent_behind_gap    = 0.0f;
    opponent_behind_offset = 1.8f;
    road_curvature         = 0.0f;
    opponent_behind_speed  = 0.0f;

    opponent_ahead_dist    = 20.0f;
    opponent_ahead_gap     = 0.0f;
    track_progress_pct     = 0.0f;

    ers_charge_pct         = 0.0f;
    ers_mode               = 0;
    mguk_recovery          = 0;
    mguk_delivery          = 0;
    ers_lap_deployed       = 0.0f;

    fl = WheelData();
    fr = WheelData();
    rl = WheelData();
    rr = WheelData();

    for (int i = 0; i < 5; i++) {
        shift_leds[i] = false;
        strncpy(lap_history[i], "--:--:--.-", sizeof(lap_history[i]));
    }
}

// -----------------------------------------------------------------------------
// WebSocket Protocol Engine Internals
// -----------------------------------------------------------------------------
namespace SimHubWs {
    static WebSocketsClient ws_client;
    
    static uint32_t ws_packet_count   = 0;
    static uint32_t ws_last_metric_ms = 0;
    static size_t   ws_last_bytes     = 0;

    static char seen_keys[64][16];
    static size_t seen_keys_count = 0;

    static void echo_key_to_serial_first_occurrence(const char* group, const char* key, JsonVariant val) {
        if (!key) return;

        for (size_t i = 0; i < seen_keys_count; i++) {
            if (strcmp(seen_keys[i], key) == 0) return;
        }

        if (seen_keys_count < 64) {
            strncpy(seen_keys[seen_keys_count], key, sizeof(seen_keys[0]) - 1);
            seen_keys[seen_keys_count][sizeof(seen_keys[0]) - 1] = '\0';
            seen_keys_count++;
        }

        char val_buf[32] = "<empty>";
        if (!val.isNull()) {
            if (val.is<const char*>()) {
                const char* s = val.as<const char*>();
                snprintf(val_buf, sizeof(val_buf), "%s", s ? s : "<null>");
            } else if (val.is<float>()) {
                snprintf(val_buf, sizeof(val_buf), "%.2f", val.as<float>());
            } else if (val.is<int>()) {
                snprintf(val_buf, sizeof(val_buf), "%d", val.as<int>());
            } else if (val.is<bool>()) {
                snprintf(val_buf, sizeof(val_buf), "%s", val.as<bool>() ? "true" : "false");
            } else {
                strcpy(val_buf, "<complex>");
            }
        }

        const char* common = ActiveTemplate::get_common_name(key);
        Serial.printf("[NEW KEY DISCOVERED #%02u] [%s] %-8s (%-10s) -> Sample Value: %s\n",
                      (unsigned)seen_keys_count,
                      group ? group : "?",
                      key,
                      common ? common : "UNKNOWN",
                      val_buf);
    }

    static void parse_component_diff(JsonObject d) {
        if (sim_data_mutex == NULL) return;

        if (xSemaphoreTake(sim_data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            live_sim_data.connected    = true;
            live_sim_data.game_running = true;

            // Safe key discovery diagnostics
            if (d.containsKey("T")) {
                JsonObject t = d["T"];
                for (JsonPair kv : t) {
                    echo_key_to_serial_first_occurrence("T", kv.key().c_str(), kv.value());
                }

                // Channel 63: Remote Screen Switch Selector
                if (t.containsKey("63")) {
                    live_sim_data.requested_page = atoi(t["63"].as<const char*>());
                } else if (t.containsKey("63r0")) {
                    live_sim_data.requested_page = atoi(t["63r0"].as<const char*>());
                }
            }

            if (d.containsKey("C")) {
                JsonObject c = d["C"];
                for (JsonPair kv : c) {
                    echo_key_to_serial_first_occurrence("C", kv.key().c_str(), kv.value());
                }
            }

            // Ingest all mapped channels into live_sim_data
            ActiveTemplate::parse_template_diff(d, live_sim_data);

            live_sim_data.last_packet_ms = millis();
            xSemaphoreGive(sim_data_mutex);
        }
    }

    static void onWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
        switch (type) {
            case WStype_DISCONNECTED:
                Serial.println("[WS] Disconnected from SimHub.");
                if (sim_data_mutex && xSemaphoreTake(sim_data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                    live_sim_data.connected = false;
                    strncpy(live_sim_data.gear, "N", sizeof(live_sim_data.gear));
                    xSemaphoreGive(sim_data_mutex);
                }
                break;

            case WStype_CONNECTED:
                Serial.printf("[WS] Connected to ws://%s:%d%s\n", 
                              app_config.simhub_ip, app_config.simhub_port, (char*)payload);
                ws_client.sendTXT("echo|");
                break;

            case WStype_TEXT: {
                // Heartbeat reply
                if (strncmp((char*)payload, "echo|", 5) == 0) {
                    ws_client.sendTXT("-1|echo");
                    break;
                }

                // Initial handshake confirmed -> register template
                if (strncmp((char*)payload, "-1|echo", 7) == 0) {
                    Serial.println("[WS] Handshake confirmed -> Dispatching registration pipeline...");
                    
                    ws_client.sendTXT("pid|-1");
                    ws_client.sendTXT("mainTemplateLoading|");

                    char reg_cmd[160];
                    snprintf(reg_cmd, sizeof(reg_cmd), "registerComponents|%s", ActiveTemplate::TEMPLATE_PATH);
                    ws_client.sendTXT(reg_cmd);

                    ws_client.sendTXT("mainTemplateLoaded|");
                    Serial.println("[WS] Registration pipeline sent. Stream active.");
                    break;
                }

                // Packet framing: <seq_id>|<json>
                char* pipe_ptr = strchr((char*)payload, '|');
                if (pipe_ptr == nullptr) break;

                *pipe_ptr = '\0';
                int seq_id = atoi((char*)payload);
                *pipe_ptr = '|';

                // Send immediate lockstep ACK back to SimHub
                char ack_buf[24];
                snprintf(ack_buf, sizeof(ack_buf), "pid|%d", seq_id);
                ws_client.sendTXT(ack_buf);

                const char* json_data = pipe_ptr + 1;
                ws_packet_count++;
                ws_last_bytes = length;

                DynamicJsonDocument doc(4096);
                DeserializationError err = deserializeJson(doc, json_data);
                if (!err && doc.containsKey("d")) {
                    parse_component_diff(doc["d"]);
                }
                break;
            }

            case WStype_BIN:
            case WStype_ERROR:
            default:
                break;
        }
    }

    static void simhub_ws_task(void* pvParameters) {
        Serial.println("[WS TASK] SimHub WebSocket client task running on Core 0.");

        char last_connected_ip[32] = {0};
        uint16_t last_connected_port = 0;

        while (true) {
            if (WiFi.status() == WL_CONNECTED) {
                if (strcmp(last_connected_ip, app_config.simhub_ip) != 0 || 
                    last_connected_port != app_config.simhub_port) {
                    
                    if (strlen(app_config.simhub_ip) > 0 && strcmp(app_config.simhub_ip, "0.0.0.0") != 0) {
                        strncpy(last_connected_ip, app_config.simhub_ip, sizeof(last_connected_ip) - 1);
                        last_connected_port = app_config.simhub_port;

                        Serial.printf("[WS] Connecting to ws://%s:%d/ws ...\n",
                                      last_connected_ip, last_connected_port);
                        
                        ws_client.disconnect();
                        ws_client.begin(last_connected_ip, last_connected_port, "/ws");
                        ws_client.onEvent(onWebSocketEvent);
                        ws_client.setReconnectInterval(2000);
                    }
                }

                ws_client.loop();

                uint32_t now = millis();
                if (now - live_sim_data.last_packet_ms > 3000 && live_sim_data.connected) {
                    if (xSemaphoreTake(sim_data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                        live_sim_data.connected = false;
                        strncpy(live_sim_data.gear, "N", sizeof(live_sim_data.gear));
                        xSemaphoreGive(sim_data_mutex);
                    }
                }

                if (now - ws_last_metric_ms >= 1000) {
                    ws_last_metric_ms = now;
                    if (ws_packet_count == 0) {
                        Serial.printf("[WS METRICS] %u fps | Last: %u B | Spd: %.1f | RPM: %d | Gear: %s\n",
                                      (unsigned)ws_packet_count, (unsigned)ws_last_bytes, 
                                      live_sim_data.speed_kmh, live_sim_data.rpm, live_sim_data.gear);
                        ws_packet_count = 0;
                    }
                }
            } else {
                vTaskDelay(pdMS_TO_TICKS(500));
            }

            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

    void start_simhub_ws_client(UBaseType_t priority, BaseType_t core) {
        xTaskCreatePinnedToCore(
            simhub_ws_task,
            "SimHubWsTask",
            8192,
            NULL,
            priority,
            NULL,
            core
        );
    }
}