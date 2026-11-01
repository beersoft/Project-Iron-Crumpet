#pragma once

// Dial bar tone styles
#define DIAL_SPEED_ON       TFT_CYAN
#define DIAL_SPEED_DIM      0x028A  // Solid Dark Cyan track

#define DIAL_RPM_ON         TFT_YELLOW
#define DIAL_RPM_DIM        0x6300  // Solid Dark Yellow track
#define DIAL_REDLINE_ON     TFT_RED
#define DIAL_REDLINE_DIM    0x4800  // Solid Dark Redline track

// Configurable Speedometer Scale (changeable for GT3, F1, Street, etc.)
inline int dial_max_speed = 200; 

static DashCardState card_dial_speed;
static DashCardState card_dial_rpm;
static DashCardState card_dial_gear;

static float last_speed_angle = 150.0f;
static float last_rpm_angle   = 150.0f;

inline void init_page_dials_sprites() { }

inline void polar_to_xy(int cx, int cy, float radius, float angle_deg, int &out_x, int &out_y) {
    float rad = angle_deg * (PI / 180.0f);
    out_x = cx + (int)(radius * cos(rad));
    out_y = cy + (int)(radius * sin(rad));
}

inline void render_page_dials_static() {
    if (current_page != PAGE_DIALS) return;

    gfx.fillScreen(DASH_BG_GLOBAL);

    card_dial_speed.reset();
    card_dial_rpm.reset();
    card_dial_gear.reset();
    last_speed_angle = 150.0f;
    last_rpm_angle   = 150.0f;

    // Center coordinates for dual circular dials
    const int cx_spd = 210, cy_spd = 215;
    const int cx_rpm = 590, cy_rpm = 215;

    // Arc dimensions
    const int r_out = 145;
    const int r_in  = 125;
    const int circle_r = r_out + 36; // 181px radius

    // -------------------------------------------------------------------------
    // 1. DIAL CIRCLE ENCLOSURES
    // -------------------------------------------------------------------------
    gfx.fillCircle(cx_spd, cy_spd, circle_r, DASH_CARD_BG);
    gfx.drawCircle(cx_spd, cy_spd, circle_r, DASH_CARD_BORDER);

    gfx.fillCircle(cx_rpm, cy_rpm, circle_r, DASH_CARD_BG);
    gfx.drawCircle(cx_rpm, cy_rpm, circle_r, DASH_CARD_BORDER);

    // -------------------------------------------------------------------------
    // 2. STATIC UNSWEPT BACKGROUND ARCS
    // -------------------------------------------------------------------------
    gfx.fillArc(cx_spd, cy_spd, r_out, r_in, 150, 390, DIAL_SPEED_DIM);

    int max_r = (live_sim_data.max_rpm > 0) ? live_sim_data.max_rpm : 8000;
    int redline_rpm = (int)(max_r * 0.90f);
    int redline_angle = 150 + (int)(((float)redline_rpm / (float)max_r) * 240.0f);

    gfx.fillArc(cx_rpm, cy_rpm, r_out, r_in, 150, redline_angle, DIAL_RPM_DIM);
    gfx.fillArc(cx_rpm, cy_rpm, r_out, r_in, redline_angle, 390, DIAL_REDLINE_DIM);

    // Inner subtle border line
    gfx.drawArc(cx_spd, cy_spd, r_in, r_in, 150, 390, DASH_CARD_BORDER);
    gfx.drawArc(cx_rpm, cy_rpm, r_in, r_in, 150, 390, DASH_CARD_BORDER);

    // -------------------------------------------------------------------------
    // 3. DIAL TICKS & NUMBERS
    // -------------------------------------------------------------------------
    // Speed Ticks & Numbers dynamically scaled to dial_max_speed
    int speed_step = (dial_max_speed > 250) ? 50 : 25;
    for (int spd = 0; spd <= dial_max_speed; spd += speed_step) {
        float angle = 150.0f + ((float)spd / (float)dial_max_speed) * 240.0f;
        int x1, y1, x2, y2, tx, ty;
        polar_to_xy(cx_spd, cy_spd, r_out + 2, angle, x1, y1);
        polar_to_xy(cx_spd, cy_spd, r_out + 12, angle, x2, y2);
        gfx.drawLine(x1, y1, x2, y2, TFT_WHITE);

        polar_to_xy(cx_spd, cy_spd, r_out + 24, angle, tx, ty);
        gfx.setFont(DASH_FONT_LABEL);
        gfx.setTextSize(0.45f);
        gfx.setTextDatum(MC_DATUM);
        gfx.setTextColor(DASH_TEXT_SLATE, DASH_CARD_BG);
        gfx.drawNumber(spd, tx, ty);
    }

    // RPM Ticks & Numbers
    int max_thousands = max_r / 1000;
    for (int k = 0; k <= max_thousands; ++k) {
        float angle = 150.0f + ((float)(k * 1000) / (float)max_r) * 240.0f;
        bool is_red = ((k * 1000) >= redline_rpm);

        int x1, y1, x2, y2, tx, ty;
        polar_to_xy(cx_rpm, cy_rpm, r_out + 2, angle, x1, y1);
        polar_to_xy(cx_rpm, cy_rpm, r_out + 12, angle, x2, y2);
        gfx.drawLine(x1, y1, x2, y2, is_red ? TFT_RED : TFT_WHITE);

        polar_to_xy(cx_rpm, cy_rpm, r_out + 24, angle, tx, ty);
        gfx.setFont(DASH_FONT_LABEL);
        gfx.setTextSize(0.55f);
        gfx.setTextDatum(MC_DATUM);
        gfx.setTextColor(is_red ? TFT_RED : TFT_WHITE, DASH_CARD_BG);
        gfx.drawNumber(k, tx, ty);
    }

    // Static Units Labels
    gfx.setFont(DASH_FONT_LABEL);
    gfx.setTextSize(0.60f);
    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(DASH_TEXT_SLATE, DASH_CARD_BG);
    gfx.drawString("KM/H", cx_spd, cy_spd + 58);
    gfx.drawString("RPM", cx_rpm, cy_rpm + 58);

    // -------------------------------------------------------------------------
    // 4. GEAR CONTAINER
    // -------------------------------------------------------------------------
    gfx.fillRoundRect(356, 312, 88, 80, 10, DASH_CARD_BG);
    gfx.drawRoundRect(356, 312, 88, 80, 10, DASH_CARD_BORDER);
}

inline void update_dials_page_ui(const SimTelemetry& snap) {
    if (current_page != PAGE_DIALS) return;

    const int cx_spd = 210, cy_spd = 215;
    const int cx_rpm = 590, cy_rpm = 215;
    const int r_out = 145;
    const int r_in  = 125;

    // -------------------------------------------------------------------------
    // 1. SPEED ARC & READOUT (Dynamically scaled to dial_max_speed)
    // -------------------------------------------------------------------------
    float spd = constrain(card_dial_speed.smoother.get_extrapolated(0.0f, (float)(dial_max_speed + 50)), 0.0f, (float)dial_max_speed);
    float target_spd_angle = 150.0f + (spd / (float)dial_max_speed) * 240.0f;

    if (fabs(target_spd_angle - last_speed_angle) >= 0.8f) {
        if (target_spd_angle > last_speed_angle) {
            gfx.fillArc(cx_spd, cy_spd, r_out, r_in, (int)last_speed_angle, (int)target_spd_angle, DIAL_SPEED_ON);
        } else {
            gfx.fillArc(cx_spd, cy_spd, r_out, r_in, (int)target_spd_angle, (int)last_speed_angle, DIAL_SPEED_DIM);
        }
        last_speed_angle = target_spd_angle;

        char s_buf[16];
        snprintf(s_buf, sizeof(s_buf), "%03d", constrain((int)spd, 0, 999));

        gfx.fillRect(cx_spd - 110, cy_spd - 60, 220, 80, DASH_CARD_BG);
        gfx.setFont(DASH_FONT_DATA_NUM);
        gfx.setTextSize(DASH_FONT_NUM_HERO_SIZE);
        gfx.setTextDatum(MC_DATUM);
        gfx.setTextColor(DIAL_SPEED_ON, DASH_CARD_BG);
        gfx.drawString(s_buf, cx_spd, cy_spd - 15);
    }
    card_dial_speed.smoother.update_target(snap.speed_kmh);

    // -------------------------------------------------------------------------
    // 2. RPM ARC & READOUT
    // -------------------------------------------------------------------------
    int max_r = (snap.max_rpm > 0) ? snap.max_rpm : 8000;
    float cur_rpm = constrain(card_dial_rpm.smoother.get_extrapolated(0.0f, 20000.0f), 0.0f, (float)max_r);
    float target_rpm_angle = 150.0f + (cur_rpm / (float)max_r) * 240.0f;
    int redline_rpm = (int)(max_r * 0.90f);
    int redline_angle = 150 + (int)(((float)redline_rpm / (float)max_r) * 240.0f);

    if (fabs(target_rpm_angle - last_rpm_angle) >= 0.8f) {
        if (target_rpm_angle > last_rpm_angle) {
            int a_start = (int)last_rpm_angle;
            int a_end   = (int)target_rpm_angle;

            if (a_start < redline_angle && a_end > redline_angle) {
                gfx.fillArc(cx_rpm, cy_rpm, r_out, r_in, a_start, redline_angle, DIAL_RPM_ON);
                gfx.fillArc(cx_rpm, cy_rpm, r_out, r_in, redline_angle, a_end, DIAL_REDLINE_ON);
            } else if (a_end <= redline_angle) {
                gfx.fillArc(cx_rpm, cy_rpm, r_out, r_in, a_start, a_end, DIAL_RPM_ON);
            } else {
                gfx.fillArc(cx_rpm, cy_rpm, r_out, r_in, a_start, a_end, DIAL_REDLINE_ON);
            }
        } else {
            int a_start = (int)target_rpm_angle;
            int a_end   = (int)last_rpm_angle;

            if (a_start < redline_angle && a_end > redline_angle) {
                gfx.fillArc(cx_rpm, cy_rpm, r_out, r_in, a_start, redline_angle, DIAL_RPM_DIM);
                gfx.fillArc(cx_rpm, cy_rpm, r_out, r_in, redline_angle, a_end, DIAL_REDLINE_DIM);
            } else if (a_end <= redline_angle) {
                gfx.fillArc(cx_rpm, cy_rpm, r_out, r_in, a_start, a_end, DIAL_RPM_DIM);
            } else {
                gfx.fillArc(cx_rpm, cy_rpm, r_out, r_in, a_start, a_end, DIAL_REDLINE_DIM);
            }
        }
        last_rpm_angle = target_rpm_angle;

        char r_buf[16];
        snprintf(r_buf, sizeof(r_buf), "%04d", constrain((int)cur_rpm, 0, 9999));

        uint16_t text_col = (cur_rpm >= redline_rpm) ? DIAL_REDLINE_ON : DIAL_RPM_ON;

        gfx.fillRect(cx_rpm - 105, cy_rpm - 55, 210, 75, DASH_CARD_BG);
        gfx.setFont(DASH_FONT_DATA_NUM);
        gfx.setTextSize(DASH_FONT_NUM_MD_SIZE);
        gfx.setTextDatum(MC_DATUM);
        gfx.setTextColor(text_col, DASH_CARD_BG);
        gfx.drawString(r_buf, cx_rpm, cy_rpm - 15);
    }
    card_dial_rpm.smoother.update_target((float)snap.rpm);

    // -------------------------------------------------------------------------
    // 3. GEAR READOUT
    // -------------------------------------------------------------------------
    uint16_t gear_col = (strcmp(snap.gear, "R") == 0) ? TFT_RED : 
                        ((strcmp(snap.gear, "N") == 0) ? TFT_GREEN : TFT_YELLOW);

    if (strcmp(snap.gear, card_dial_gear.text_cache) != 0 || gear_col != card_dial_gear.color_cache) {
        strncpy(card_dial_gear.text_cache, snap.gear, sizeof(card_dial_gear.text_cache) - 1);
        card_dial_gear.color_cache = gear_col;

        gfx.fillRect(360, 316, 80, 72, DASH_CARD_BG);
        gfx.setFont(DASH_FONT_DATA_NUM);
        gfx.setTextSize(DASH_FONT_NUM_HERO_SIZE * 0.85f);
        gfx.setTextDatum(MC_DATUM);
        gfx.setTextColor(gear_col, DASH_CARD_BG);
        gfx.drawString(snap.gear, 400, 352);
    }
}