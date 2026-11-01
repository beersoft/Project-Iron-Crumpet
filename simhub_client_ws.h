#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Individual Corner Wheel Metrics
// -----------------------------------------------------------------------------
struct WheelData {
    float temp_c   = 0.0f;
    float brake_c  = 0.0f;
    float pressure = 0.0f;
};

// -----------------------------------------------------------------------------
// Sector Timing Information
// -----------------------------------------------------------------------------
struct SectorInfo {
    char    time_str[12] = {0};
    uint8_t status       = 0; // 0=None, 1=Yellow, 2=Green, 3=Purple
};

// -----------------------------------------------------------------------------
// Core Live SimHub Telemetry State
// -----------------------------------------------------------------------------
struct SimTelemetry {
    // Connection & Session State
    bool connected      = false;
    bool game_running   = false;
    bool in_race        = false;
    bool game_paused    = false;
    bool game_replay    = false;
    char game_name[32]  = {0};
    char track_name[32] = {0};
    char car_name[32]   = "NO CAR DETECTED";
    char car_class[24]  = "SHITBOX";

    // Race Context & Lap Timing
    int  position              = 0;
    int  current_lap           = 0;
    int  total_laps            = 0;
    char current_lap_time[24] = "--:--:--.-";
    char last_lap_time[24]    = "--:--:--.-";
    char best_lap_time[24]    = "--:--:--.-";
    char all_time_best[24]    = "--:--:--.-";

    char lap_history[5][24]   = {
        "--:--:--.-", "--:--:--.-", "--:--:--.-", "--:--:--.-", "--:--:--.-"
    };

    // Remote Navigation
    int requested_page        = -1;

    // Splits & Deltas
    float delta_to_best           = 0.0f;
    float session_best_live_delta = 0.0f;
    float lap_delta_vs_prev_best  = 0.0f;
    float self_split_delta        = 0.0f;
    bool  delta_valid             = false;

    SectorInfo sectors[3];

    // Engine & Drivetrain
    float speed_kmh   = 0.0f;
    int   rpm         = 0;
    int   max_rpm     = 8000;
    char  gear[4]     = "N";
    float turbo_boost = 0.0f;

    // Driver Assists & Intervention Flags
    bool abs_available         = false;
    int  abs_level             = 0;
    bool abs_active            = false;

    bool tc_available          = false;
    int  tc_level              = 0;
    bool tc_active             = false;

    bool pit_limiter_available = false;
    bool pit_limiter_on        = false;

    // Lighting & Wipers
    bool lights_available      = false;
    bool headlights_on         = false;
    bool highbeam_flash_on     = false;

    bool wipers_available      = false;
    bool wipers_on             = false;

    // Powertrain Tuning & Flags
    bool  ignition_available   = false;
    bool  engine_running       = false;
    int   tc_cut               = 0;
    int   tc_slip              = 0;
    int   engine_map           = 0;
    float brake_bias           = 0.0f;
    float brake_migration      = 0.0f;
    bool  handbrake_active     = false;
    char  flag_name[16]        = "None";

    // Fluids, Fuel & Environmental Metrics
    float fuel_pct             = 0.0f;
    float battery_charge_pct   = 0.0f;
    float air_temperature      = 0.0f;
    float water_temperature    = 0.0f;
    float oil_temperature      = 0.0f;
    float oil_pressure         = 0.0f;
    float fuel_pressure        = 0.0f;
    float road_temperature     = 0.0f;
    char  session_type[16]     = "Race";

    // Indicators & Blindspot Spotters
    bool turn_indicator_left   = false;
    bool turn_indicator_right  = false;
    bool spotter_left          = false;
    bool spotter_right         = false;

    // Rear Traffic & 3D Mirror Pipeline (Channels 49 - 53)
    float opponent_behind_dist   = 12.0f;
    float opponent_behind_gap    = 0.0f;
    float opponent_behind_offset = 1.8f;
    float road_curvature         = 0.0f;
    float opponent_behind_speed  = 0.0f;

    // Forward Proximity Radar (Channels 54 - 55)
    float opponent_ahead_dist    = 20.0f;
    float opponent_ahead_gap     = 0.0f;

    // Track Position & Map Progress
    float track_progress_pct     = 0.0f;

    // Hybrid ERS & MGU-K Telemetry (Channels 57 - 61)
    float ers_charge_pct         = 0.0f;
    int   ers_mode               = 0;
    int   mguk_recovery          = 0;
    int   mguk_delivery          = 0;
    float ers_lap_deployed       = 0.0f;

    // Wheel Sub-Structures
    WheelData fl;
    WheelData fr;
    WheelData rl;
    WheelData rr;

    bool     shift_leds[5]     = {false, false, false, false, false};
    uint32_t last_packet_ms    = 0;

    // -------------------------------------------------------------------------
    // Resolves gauge max speed based on car class / type conventions
    // -------------------------------------------------------------------------
    float get_conventional_max_speed() const {
        if (strlen(car_class) == 0) return 300.0f;

        if (strcasestr(car_class, "F1") || strcasestr(car_class, "Formula") ||
            strcasestr(car_class, "LMP") || strcasestr(car_class, "Hypercar") ||
            strcasestr(car_class, "GTP")) {
            return 380.0f;
        }

        if (strcasestr(car_class, "GT3") || strcasestr(car_class, "GTE") ||
            strcasestr(car_class, "GT4") || strcasestr(car_class, "Cup") ||
            strcasestr(car_class, "GT")) {
            return 300.0f;
        }

        if (strcasestr(car_class, "TCR") || strcasestr(car_class, "Touring") ||
            strcasestr(car_class, "Road") || strcasestr(car_class, "Club")) {
            return 240.0f;
        }

        return 300.0f;
    }

    // -------------------------------------------------------------------------
    // Resolves gauge redline & max RPM bounds based on car class / type
    // -------------------------------------------------------------------------
    float get_conventional_max_rpm() const {
        if (max_rpm > 2000) {
            return (float)max_rpm;
        }

        if (strlen(car_class) == 0) return 8500.0f;

        if (strcasestr(car_class, "F1") || strcasestr(car_class, "Formula") || 
            strcasestr(car_class, "Indy")) {
            return 15000.0f;
        }

        if (strcasestr(car_class, "LMP") || strcasestr(car_class, "Hypercar") ||
            strcasestr(car_class, "GTP") || strcasestr(car_class, "DPi")) {
            return 10000.0f;
        }

        if (strcasestr(car_class, "GT3") || strcasestr(car_class, "GTE") || 
            strcasestr(car_class, "Cup") || strcasestr(car_class, "GT")) {
            return 8500.0f;
        }

        if (strcasestr(car_class, "GT4") || strcasestr(car_class, "TCR") || 
            strcasestr(car_class, "Touring")) {
            return 7500.0f;
        }

        if (strcasestr(car_class, "Road") || strcasestr(car_class, "Club") ||
            strcasestr(car_class, "Miata") || strcasestr(car_class, "MX5")) {
            return 7000.0f;
        }

        return 8500.0f;
    }

    void reset_to_zero();
};

// -----------------------------------------------------------------------------
// Global Instance Declarations
// -----------------------------------------------------------------------------
extern SimTelemetry live_sim_data;
extern SemaphoreHandle_t sim_data_mutex;

// -----------------------------------------------------------------------------
// SimHub WebSocket Client Control Namespace
// -----------------------------------------------------------------------------
namespace SimHubWs {
    void start_simhub_ws_client(UBaseType_t priority = 2, BaseType_t core = 0);
}