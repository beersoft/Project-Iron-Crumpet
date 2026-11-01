// telemetry_smoother.h - Rev 1.0.0
// Client-side dead-reckoning extrapolation and local lap timing
#pragma once

#include <Arduino.h>

class TelemetryExtrapolator {
public:
    float current_val = 0.0f;
    float target_val = 0.0f;
    float rate_per_ms = 0.0f;
    uint32_t last_packet_time = 0;

    void update_target(float new_val) {
        uint32_t now = millis();
        if (last_packet_time > 0) {
            uint32_t dt = now - last_packet_time;
            if (dt > 5 && dt < 250) {
                float raw_rate = (new_val - target_val) / (float)dt;
                // Soft filter on rate to prevent erratic jumps from network jitter
                rate_per_ms = (rate_per_ms * 0.4f) + (raw_rate * 0.6f);
            } else {
                rate_per_ms = 0.0f;
            }
        }
        target_val = new_val;
        last_packet_time = now;

        // Instant drop on gearshifts, lift-off, or heavy braking
        if (new_val < current_val - 250.0f) {
            current_val = new_val;
            rate_per_ms = 0.0f;
        }
    }

    float get_extrapolated(float min_clamp, float max_clamp, uint32_t max_extrapolate_ms = 90) {
        uint32_t now = millis();
        uint32_t dt = now - last_packet_time;

        if (dt < max_extrapolate_ms && fabs(rate_per_ms) > 0.001f) {
            float projected = target_val + (rate_per_ms * dt);
            // Smoothly converge current display toward projected value
            current_val += (projected - current_val) * 0.35f;
        } else {
            // Decay toward target if packet stream pauses
            current_val += (target_val - current_val) * 0.25f;
        }

        return constrain(current_val, min_clamp, max_clamp);
    }
};

class LocalLapTimer {
public:
    uint32_t latched_sim_ms = 0;
    uint32_t latched_local_ms = 0;
    int latched_lap_num = -1;
    bool active = false;

    // Parses "01:23.456", "00:01:23.456", or "23.456" into milliseconds
    static uint32_t parse_time_str(const char* str) {
        if (!str || strlen(str) < 3) return 0;
        int p1 = 0, p2 = 0, p3 = 0, p4 = 0;
        int colons = 0;
        for (const char* p = str; *p; p++) if (*p == ':') colons++;

        if (colons == 2) {
            // HH:MM:SS.mmm
            sscanf(str, "%d:%d:%d.%d", &p1, &p2, &p3, &p4);
            return (p1 * 3600000UL) + (p2 * 60000UL) + (p3 * 1000UL) + p4;
        } else if (colons == 1) {
            // MM:SS.mmm
            sscanf(str, "%d:%d.%d", &p1, &p2, &p3);
            return (p1 * 60000UL) + (p2 * 1000UL) + p3;
        }
        return 0;
    }

    void sync(const char* sim_time_str, int current_lap, bool is_in_race) {
        if (!is_in_race || !sim_time_str || strlen(sim_time_str) == 0) {
            active = false;
            return;
        }

        uint32_t incoming_ms = parse_time_str(sim_time_str);
        uint32_t now = millis();

        // New lap or initial lock
        if (current_lap != latched_lap_num || !active || incoming_ms < 500) {
            latched_lap_num = current_lap;
            latched_sim_ms = incoming_ms;
            latched_local_ms = now;
            active = true;
            return;
        }

        // Clock drift compensation (soft resync if drift < 250ms, hard snap if larger)
        uint32_t current_projected = latched_sim_ms + (now - latched_local_ms);
        int32_t drift = (int32_t)incoming_ms - (int32_t)current_projected;

        if (abs(drift) > 250) {
            latched_sim_ms = incoming_ms;
            latched_local_ms = now;
        } else {
            // Micro-nudge the baseline
            latched_sim_ms += (int32_t)(drift * 0.2f);
        }
    }

    void format_current(char* dest, size_t dest_len, const char* fallback_str) {
        if (!active) {
            strncpy(dest, (fallback_str && strlen(fallback_str) > 0) ? fallback_str : "--:--.---", dest_len - 1);
            return;
        }

        uint32_t now = millis();
        uint32_t total_ms = latched_sim_ms + (now - latched_local_ms);

        uint32_t minutes = (total_ms / 60000UL) % 60;
        uint32_t seconds = (total_ms / 1000UL) % 60;
        uint32_t millis_rem = total_ms % 1000UL;

        snprintf(dest, dest_len, "%02u:%02u.%03u", minutes, seconds, millis_rem);
    }
};

extern TelemetryExtrapolator smooth_speed;
extern TelemetryExtrapolator smooth_rpm;
extern LocalLapTimer local_lap_clock;
