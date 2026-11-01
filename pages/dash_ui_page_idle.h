#pragma once


void switch_dashboard_page(DashboardPage target_page);

static DashCardState card_sim_status;
static DashCardState card_link_status;

inline void init_page_idle_sprites() { }

inline void render_page_idle_static() {
    if (current_page != PAGE_IDLE) return;
    gfx.fillScreen(DASH_BG_GLOBAL);

    card_sim_status.reset();
    card_link_status.reset();

    // 1. Static Header Card (Row 1, 12 cols wide, 1 row high)
  //  draw_element_card(1, 1, 12, 1, "", "STANDBY & AUTO-DETECT ENGINE", TFT_YELLOW);

    // 2. Active Simulator Card (Rows 2-3, 12 cols wide, 2 rows high)
    draw_element_card(2, 1, 12, 2, "", "WAITING FOR SIM DATA STREAM", TFT_WHITE);

    // 3. Telemetry Link Status Card (Rows 4-5, 12 cols wide, 2 rows high)
    draw_element_card(4, 1, 12, 2, "",   "SEARCHING NETWORK...", TFT_ORANGE);

    // Bottom Hint Banner mapped cleanly to row 6
    int32_t fx, fy, fw, fh;
    grid_to_pixels(6, 1, 1, 12, fx, fy, fw, fh);
    
    gfx.fillRect(fx, fy, fw, fh, DASH_CARD_BG);
    gfx.drawRoundRect(fx, fy, fw, fh, 6, DASH_CARD_BORDER);

    gfx.setFont(DASH_FONT_LABEL);
    gfx.setTextSize(0.45f);
    gfx.setTextColor(0x8410 /* DASH_TEXT_SLATE */, DASH_CARD_BG);
    gfx.setTextDatum(MC_DATUM);
    gfx.drawString("Touch screen to cycle pages manually. Telemetry packet will auto-wake display.", fx + (fw / 2), fy + (fh / 2));
}

inline void update_idle_page_ui(const SimTelemetry& snap) {
    if (current_page != PAGE_IDLE) return;

    if (snap.connected && (snap.game_running || snap.in_race || snap.rpm > 200)) {
        switch_dashboard_page(PAGE_RACE);
        return;
    }

    if (snap.connected) {
        const char* g_name = (strlen(snap.game_name) > 0) ? snap.game_name : "GAME RUNNING";
        dash_element(PAGE_IDLE, card_sim_status, 2, 1, 12, 2, "ACTIVE SIMULATOR", g_name, TFT_YELLOW);

        char session_buf[64];
        if (snap.game_paused) {
            snprintf(session_buf, sizeof(session_buf), "ONLINE - SESSION PAUSED");
        } else if (snap.in_race) {
            snprintf(session_buf, sizeof(session_buf), "ONLINE - ON TRACK (RACE)");
        } else {
            snprintf(session_buf, sizeof(session_buf), "ONLINE - IN PITS / STANDBY");
        }
        dash_element(PAGE_IDLE, card_link_status, 4, 1, 12, 2, "", session_buf, TFT_GREEN);
    } else {
        static uint32_t last_dot_time = 0;
        static int dot_count = 0;
        if (millis() - last_dot_time > 400) {
            last_dot_time = millis();
            dot_count = (dot_count + 1) % 4;

            char dots[8] = "";
            for (int i = 0; i < dot_count; i++) strcat(dots, " .");

            char search_str[64];
            snprintf(search_str, sizeof(search_str), "SEARCHING SUBNET%s", dots);
            dash_element(PAGE_IDLE, card_link_status, 4, 1, 12, 2, "", search_str, TFT_ORANGE);
        }
    }
}