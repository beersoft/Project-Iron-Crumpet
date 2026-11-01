
// dash_ui_alerts.h
#pragma once


#define ALERT_DARK_RED  0x8800  // Deep dark red (RGB ~ 136, 0, 0)

// -----------------------------------------------------------------------------
// Alert State Tracking
// -----------------------------------------------------------------------------
static uint32_t redline_start_ms = 0;
static uint32_t last_manifold_trigger = 0;

inline void init_alerts_ui() {
    active_alert.active = false;
    active_alert.title[0] = '\0';
    active_alert.value[0] = '\0';
    active_alert.expires = 0;
    active_alert.color = TFT_BLACK;
}

inline void render_setting_alert() {
    if (!active_alert.active) return;

    // Centered overlay card across Rows 2-5, Cols 2-11
    int32_t x, y, w, h;
    grid_to_pixels(2, 2, 4, 10, x, y, w, h);

    // Light grey popup fill with solid black border
    gfx.fillRect(x, y, w, h, TFT_LIGHTGRAY);
    gfx.drawRoundRect(x, y, w, h, 8, TFT_BLACK);
    gfx.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 8, TFT_BLACK);

    // Larger Alert Title in Dark Red
    gfx.setFont(DASH_FONT_DATA);
    gfx.setTextSize(1.35f);
    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(ALERT_DARK_RED, TFT_LIGHTGRAY);
    gfx.drawString(active_alert.title, x + (w / 2), y + 36);

    // Main alert string in solid black
    gfx.setTextColor(TFT_BLACK, TFT_LIGHTGRAY);

    if (strlen(active_alert.value) > 12) {
        gfx.setTextSize(1.1f);
    } else {
        gfx.setTextSize(1.6f);
    }

    gfx.drawString(active_alert.value, x + (w / 2), y + (h / 2) + 20);
}

// -----------------------------------------------------------------------------
// Sustained Redline Monitor (Danger to Manifold)
// -----------------------------------------------------------------------------
inline void check_danger_to_manifold_alert(const SimTelemetry& snap) {
    uint32_t now = millis();
    int redline_thresh = (snap.max_rpm > 0) ? (int)(snap.max_rpm * 0.95f) : 6800;

    if (snap.connected && (snap.game_running || snap.in_race) && snap.rpm >= redline_thresh) {
        if (redline_start_ms == 0) {
            redline_start_ms = now;
        } else if (now - redline_start_ms >= 3000) {
            if (now - last_manifold_trigger > 6000) {
                last_manifold_trigger = now;
                redline_start_ms = now - 1500;

                active_alert.active = true;
                strncpy(active_alert.title, "CRITICAL WARNING", sizeof(active_alert.title) - 1);
                strncpy(active_alert.value, "DANGER TO MANIFOLD", sizeof(active_alert.value) - 1);
                active_alert.color = TFT_BLACK;
                active_alert.expires = now + 2500;

                render_setting_alert();
            }
        }
    } else {
        redline_start_ms = 0;
    }
}

// -----------------------------------------------------------------------------
// Car Setting Shift Detection (Brake Bias, TC, ABS, Engine Map)
// -----------------------------------------------------------------------------
inline void process_driver_alerts(const SimTelemetry& snap) {
    if (active_alert.active) return;

    check_danger_to_manifold_alert(snap);
    if (active_alert.active) return;

    if (!snap.connected) return;

    uint32_t now = millis();

    if (ui_cache.brake_bias > 0.0f && fabs(snap.brake_bias - ui_cache.brake_bias) >= 0.25f) {
        ui_cache.brake_bias = snap.brake_bias;
        active_alert.active = true;
        strncpy(active_alert.title, "BRAKE BIAS", sizeof(active_alert.title) - 1);
        snprintf(active_alert.value, sizeof(active_alert.value), "%.1f %%", snap.brake_bias);
        active_alert.color = TFT_BLACK;
        active_alert.expires = now + 1500;
        render_setting_alert();
        return;
    }

    if (ui_cache.tc_level >= 0 && snap.tc_level != ui_cache.tc_level) {
        ui_cache.tc_level = snap.tc_level;
        active_alert.active = true;
        strncpy(active_alert.title, "TRACTION CONTROL", sizeof(active_alert.title) - 1);
        snprintf(active_alert.value, sizeof(active_alert.value), "TC %d", snap.tc_level);
        active_alert.color = TFT_BLACK;
        active_alert.expires = now + 1500;
        render_setting_alert();
        return;
    }

    if (ui_cache.tc_cut >= 0 && snap.tc_cut != ui_cache.tc_cut) {
        ui_cache.tc_cut = snap.tc_cut;
        active_alert.active = true;
        strncpy(active_alert.title, "TC CUT", sizeof(active_alert.title) - 1);
        snprintf(active_alert.value, sizeof(active_alert.value), "%d", snap.tc_cut);
        active_alert.color = TFT_BLACK;
        active_alert.expires = now + 1500;
        render_setting_alert();
        return;
    }

    if (ui_cache.tc_slip >= 0 && snap.tc_slip != ui_cache.tc_slip) {
        ui_cache.tc_slip = snap.tc_slip;
        active_alert.active = true;
        strncpy(active_alert.title, "TC SLIP", sizeof(active_alert.title) - 1);
        snprintf(active_alert.value, sizeof(active_alert.value), "%d", snap.tc_slip);
        active_alert.color = TFT_BLACK;
        active_alert.expires = now + 1500;
        render_setting_alert();
        return;
    }

    if (ui_cache.abs_level >= 0 && snap.abs_level != ui_cache.abs_level) {
        ui_cache.abs_level = snap.abs_level;
        active_alert.active = true;
        strncpy(active_alert.title, "ANTI-LOCK BRAKES", sizeof(active_alert.title) - 1);
        snprintf(active_alert.value, sizeof(active_alert.value), "ABS %d", snap.abs_level);
        active_alert.color = TFT_BLACK;
        active_alert.expires = now + 1500;
        render_setting_alert();
        return;
    }

    if (ui_cache.engine_map >= 0 && snap.engine_map != ui_cache.engine_map) {
        ui_cache.engine_map = snap.engine_map;
        active_alert.active = true;
        strncpy(active_alert.title, "ENGINE MAP", sizeof(active_alert.title) - 1);
        snprintf(active_alert.value, sizeof(active_alert.value), "MAP %d", snap.engine_map);
        active_alert.color = TFT_BLACK;
        active_alert.expires = now + 1500;
        render_setting_alert();
        return;
    }
}
