#pragma once


inline const char* get_active_game_name() {
    if (strlen(ui_cache.game) > 0) return ui_cache.game;
    if (strlen(live_sim_data.game_name) > 0) return live_sim_data.game_name;
    return "STANDBY";
}

inline void init_page_stats_sprites() { }

inline void render_page_stats_static() {
    if (current_page != PAGE_STATS) return;
    gfx.fillScreen(DASH_BG_GLOBAL);

    // Terminal Diagnostic Container mapped to Rows 1-5, Cols 1-12
    int32_t x, y, w, h;
    grid_to_pixels(1, 1, 5, 12, x, y, w, h);
    gfx.fillRect(x, y, w, h, DASH_CARD_BG);
    gfx.drawRoundRect(x, y, w, h, 6, DASH_CARD_BORDER);

    // Using Orbitron font header style for consistency across pages
    gfx.setFont(&Orbitron_Bold_20);
    gfx.setTextSize(0.5f);
    gfx.setTextDatum(TL_DATUM);
    gfx.setTextColor(0x8410 /* DASH_TEXT_SLATE */, DASH_CARD_BG);
    gfx.drawString("SYSTEM DIAGNOSTICS & TELEMETRY STATUS", x + 12, y + 12);

    int line_y = y + 44;
    int line_spacing = 20;

    // Switch to Font0 with clean scaling for the terminal lines
    gfx.setFont(&fonts::Font0);
    gfx.setTextSize(1);
    gfx.setTextDatum(TL_DATUM);

    gfx.setTextColor(TFT_WHITE, DASH_CARD_BG);
    gfx.drawString("> ESP32-S3 SimHub Dashboard v2.4 initialized", x + 16, line_y);

    line_y += line_spacing;
    gfx.drawString("> LovyanGFX RGB 800x480 Driver: OK (DMA Enabled)", x + 16, line_y);

    line_y += line_spacing;
    char mem_str[64];
    snprintf(mem_str, sizeof(mem_str), "> Free Heap: %u KB | Free PSRAM: %u KB", 
             ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024);
    gfx.drawString(mem_str, x + 16, line_y);

    line_y += line_spacing;
    char net_str[96];
    if (WiFi.status() == WL_CONNECTED) {
        snprintf(net_str, sizeof(net_str), "> WiFi: Connected (%s) | IP: %s | RSSI: %d dBm",
                 WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
        gfx.setTextColor(TFT_GREEN, DASH_CARD_BG);
    } else {
        snprintf(net_str, sizeof(net_str), "> WiFi: Disconnected / Offline Mode");
        gfx.setTextColor(TFT_YELLOW, DASH_CARD_BG);
    }
    gfx.drawString(net_str, x + 16, line_y);

    line_y += line_spacing;
    char ep_str[64];
    snprintf(ep_str, sizeof(ep_str), "> SimHub Target: %s:%d | Game: %s", 
             app_config.simhub_ip, app_config.simhub_port, get_active_game_name());
    gfx.setTextColor(TFT_LIGHTGRAY, DASH_CARD_BG);
    gfx.drawString(ep_str, x + 16, line_y);

    line_y += line_spacing;
    char telem_log[96];
    if (live_sim_data.connected) {
        snprintf(telem_log, sizeof(telem_log), "[TELEM] Speed: %5.1f km/h | RPM: %5d / %5d | Gear: %s | Lap: %d",
                 live_sim_data.speed_kmh, live_sim_data.rpm, live_sim_data.max_rpm, live_sim_data.gear, live_sim_data.current_lap);
        gfx.setTextColor(TFT_GREEN, DASH_CARD_BG);
    } else {
        snprintf(telem_log, sizeof(telem_log), "[TELEM] SimHub Disconnected / Waiting...");
        gfx.setTextColor(TFT_ORANGE, DASH_CARD_BG);
    }
    gfx.drawString(telem_log, x + 16, line_y);
}

inline void update_stats_page_ui(const SimTelemetry& snap) {
    if (current_page != PAGE_STATS) return;

    static uint32_t last_diag_update = 0;
    uint32_t now = millis();

    // Refresh live telemetry log line at a steady rate to echo serial output
    if (now - last_diag_update > 250) {
        last_diag_update = now;

        int32_t x, y, w, h;
        grid_to_pixels(1, 1, 5, 12, x, y, w, h);

        int line_y = y + 44 + (20 * 5);
        gfx.fillRect(x + 14, line_y - 2, w - 28, 18, DASH_CARD_BG);
        
        gfx.setFont(&fonts::Font0);
        gfx.setTextSize(1);
        gfx.setTextDatum(TL_DATUM);

        char telem_log[96];
        if (snap.connected) {
            snprintf(telem_log, sizeof(telem_log), "[TELEM] Speed: %5.1f km/h | RPM: %5d / %5d | Gear: %s | Lap: %d",
                     snap.speed_kmh, snap.rpm, snap.max_rpm, snap.gear, snap.current_lap);
        } else {
            snprintf(telem_log, sizeof(telem_log), "[TELEM] SimHub Disconnected / Waiting...");
        }

        gfx.setTextColor(snap.connected ? TFT_GREEN : TFT_ORANGE, DASH_CARD_BG);
        gfx.drawString(telem_log, x + 16, line_y);
    }
}