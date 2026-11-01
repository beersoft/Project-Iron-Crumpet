// dash_ui_page_tyres.h
#pragma once


// Cache struct to store last rendered values and prevent redundant redraws
struct WheelRenderCache {
    float temp_c   = -999.0f;
    float brake_c  = -999.0f;
    float pressure = -999.0f;
};

inline WheelRenderCache last_fl_cache;
inline WheelRenderCache last_fr_cache;
inline WheelRenderCache last_rl_cache;
inline WheelRenderCache last_rr_cache;

inline void init_page_tyres_sprites() {
}

// Draw the static container skeleton once
inline void draw_grid_wheel_card_skeleton(int row, int col, const char* label) {
    int32_t x, y, w_px, h_px;
    grid_to_pixels(row, col, 3, 6, x, y, w_px, h_px);

    // Card background, boundary & default accent line
    gfx.fillRect(x, y, w_px, h_px, DASH_CARD_BG);
    gfx.drawRoundRect(x, y, w_px, h_px, 8, DASH_CARD_BORDER);
   // gfx.drawFastHLine(x + 16, y, w_px - 32, DASH_CARD_BORDER);

    // Header: Wheel Position Title
    draw_card_label(&gfx, x + 16, y + 10, label, TL_DATUM, TFT_WHITE);

    // Inner horizontal splitters
    int row1_y = y + 42;
    int row2_y = y + 84;
    int row3_y = y + 126;

    //gfx.drawFastHLine(x + 16, row1_y + 32, w_px - 32, DASH_CARD_SUBTLE);
   // gfx.drawFastHLine(x + 16, row2_y + 32, w_px - 32, DASH_CARD_SUBTLE);

    // Static Row Descriptors with units (allows Font7 for purely numeric values)
    draw_card_label(&gfx, x + 16, row1_y + 8, "TYRE TEMP (C)",  TL_DATUM, DASH_TEXT_SLATE);
    draw_card_label(&gfx, x + 16, row2_y + 8, "BRAKE DISC (C)", TL_DATUM, DASH_TEXT_SLATE);
    draw_card_label(&gfx, x + 16, row3_y + 8, "PRESSURE (PSI)", TL_DATUM, DASH_TEXT_SLATE);
}

// Selectively update dynamic figures only when they actually change
inline void update_wheel_card_data(int row, int col, const WheelData& current, WheelRenderCache& cached) {
    int32_t x, y, w_px, h_px;
    grid_to_pixels(row, col, 3, 6, x, y, w_px, h_px);

    int row1_y = y + 42;
    int row2_y = y + 84;
    int row3_y = y + 126;

    // 1. Tyre Temperature (Font 7)
    if (fabs(current.temp_c - cached.temp_c) >= 0.2f) {
        cached.temp_c = current.temp_c;
        uint16_t temp_col = get_tyre_temp_color(current.temp_c);

        // Update card accent line
        //gfx.drawFastHLine(x + 16, y, w_px - 32, temp_col);

        // Wipe only the data value rectangle
        gfx.fillRect(x + w_px - 140, row1_y + 2, 124, 28, DASH_CARD_BG);

        char t_str[16];
        snprintf(t_str, sizeof(t_str), "%.1f", current.temp_c);
        draw_card_data(&gfx, x + w_px - 18, row1_y + 14, t_str, DASH_FONT_NUM_SM_SIZE, temp_col, MR_DATUM, DASH_FONT_DATA_NUM);
    }

    // 2. Brake Temperature (Font 7)
    if (fabs(current.brake_c - cached.brake_c) >= 2.0f) {
        cached.brake_c = current.brake_c;
        uint16_t brake_col = get_brake_temp_color(current.brake_c);

        gfx.fillRect(x + w_px - 140, row2_y + 2, 124, 28, DASH_CARD_BG);

        char b_str[16];
        snprintf(b_str, sizeof(b_str), "%d", (int)current.brake_c);
        draw_card_data(&gfx, x + w_px - 18, row2_y + 14, b_str, DASH_FONT_NUM_SM_SIZE, brake_col, MR_DATUM, DASH_FONT_DATA_NUM);
    }

    // 3. Tyre Pressure (Font 7)
    if (fabs(current.pressure - cached.pressure) >= 0.1f) {
        cached.pressure = current.pressure;

        gfx.fillRect(x + w_px - 140, row3_y + 2, 124, 28, DASH_CARD_BG);

        char p_str[16];
        snprintf(p_str, sizeof(p_str), "%.1f", current.pressure);
        draw_card_data(&gfx, x + w_px - 18, row3_y + 14, p_str, DASH_FONT_NUM_SM_SIZE, TFT_WHITE, MR_DATUM, DASH_FONT_DATA_NUM);
    }
}

inline void render_page_tyres_static() {
    gfx.fillScreen(DASH_BG_GLOBAL);

    // Invalidate cached values so initial values render on page switch
    last_fl_cache = {-999.0f, -999.0f, -999.0f};
    last_fr_cache = {-999.0f, -999.0f, -999.0f};
    last_rl_cache = {-999.0f, -999.0f, -999.0f};
    last_rr_cache = {-999.0f, -999.0f, -999.0f};

    draw_grid_wheel_card_skeleton(1, 1, "FRONT LEFT");
    draw_grid_wheel_card_skeleton(1, 7, "FRONT RIGHT");
    draw_grid_wheel_card_skeleton(4, 1, "REAR LEFT");
    draw_grid_wheel_card_skeleton(4, 7, "REAR RIGHT");
}

inline void update_tyres_page_ui(const SimTelemetry& snap) {
    // Rate limit to 10 Hz (100 ms) to avoid bus saturation and screen flicker
    static uint32_t last_tyre_update = 0;
    uint32_t now = millis();
    if (now - last_tyre_update < 100) {
        return;
    }
    last_tyre_update = now;

    update_wheel_card_data(1, 1, snap.fl, last_fl_cache);
    update_wheel_card_data(1, 7, snap.fr, last_fr_cache);
    update_wheel_card_data(4, 1, snap.rl, last_rl_cache);
    update_wheel_card_data(4, 7, snap.rr, last_rr_cache);
}
