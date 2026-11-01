#pragma once

// Card state persistent memory
static DashCardState card_speed;
static DashCardState card_gear;
static DashCardState card_rpm;
static DashCardState card_lap;
static DashCardState card_pos;
static DashCardState card_curr_lap;

inline void init_page_race_sprites() { }

inline void render_page_race_static() {
    if (current_page != PAGE_RACE) return;

    gfx.fillScreen(DASH_BG_GLOBAL);

    // Reset card caches so they force a fresh paint on first cycle
    card_speed.reset();
    card_gear.reset();
    card_rpm.reset();
    card_lap.reset();
    card_pos.reset();
    card_curr_lap.reset();

    // Use draw_element_card_old for the top 3 hero cards to keep numbers large & centered
    draw_element_card(1, 1, 4, 5, "", "000", TFT_CYAN);
    draw_element_card(1, 5, 4, 5, "",  "N",   TFT_GREEN);
    draw_element_card(1, 9, 4, 5, "",   "0000", TFT_MAGENTA);

    // Bottom row cards
    draw_element_card(6, 1, 4, 1, "",   "0", TFT_GREEN);
    draw_element_card(6, 5, 4, 1, "",   "0", TFT_ORANGE);
    draw_element_card(6, 9, 4, 1, "",  "--:--.---", TFT_WHITE);
}

inline void update_race_page_ui(const SimTelemetry& snap) {
    if (current_page != PAGE_RACE) return;

    // Dynamic Gear Color
    uint16_t gear_col = (strcmp(snap.gear, "R") == 0) ? TFT_RED : 
                        ((strcmp(snap.gear, "N") == 0) ? TFT_GREEN : TFT_YELLOW);

    // Dynamic RPM Color
    int max_r = snap.max_rpm > 0 ? snap.max_rpm : 8000;
    uint16_t rpm_col = (card_rpm.current_val > (max_r * 0.92f)) ? TFT_RED : 
                       ((card_rpm.current_val > (max_r * 0.78f)) ? TFT_YELLOW : TFT_WHITE);

    // Format current live running lap time from the synchronized clock
    char cur_lap[24];
    local_lap_clock.format_current(cur_lap, sizeof(cur_lap), snap.current_lap_time);

    // -------------------------------------------------------------------------
    // DECLARATIVE PIPELINE WITH PAGE GUARDS
    // -------------------------------------------------------------------------
    dash_element(PAGE_RACE, card_speed, 1, 1, 4, 5, "", snap.speed_kmh, 0.0f, 300.0f, 1.0f, "%03d", TFT_CYAN);
    dash_element(PAGE_RACE, card_gear,  1, 5, 4, 5, "",  snap.gear, gear_col);
    dash_element(PAGE_RACE, card_rpm,   1, 9, 4, 5, "",   (float)snap.rpm, 0.0f, 9999.0f, 20.0f, "%04d", rpm_col);

    dash_element(PAGE_RACE, card_lap,   6, 1, 4, 1, "",   snap.current_lap, TFT_GREEN);
    dash_element(PAGE_RACE, card_pos,   6, 5, 4, 1, "",   snap.position, TFT_ORANGE);
    dash_element(PAGE_RACE, card_curr_lap, 6, 9, 4, 1, "", cur_lap, TFT_WHITE);
}