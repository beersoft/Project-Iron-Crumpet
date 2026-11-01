#pragma once


// Persistent state cache for the DRIVE page
static DashCardState card_drive_fuel;
static DashCardState card_drive_fuel_toadd;
static DashCardState card_drive_speed;
static DashCardState card_drive_last_lap;
static DashCardState card_drive_fuel_last;
static DashCardState card_drive_laps_2_go;
static DashCardState card_drive_gear;
static DashCardState card_drive_best_lap;
static DashCardState card_drive_lap;
static DashCardState card_drive_pos;
static DashCardState card_drive_bias;
static DashCardState card_drive_delta;
static DashCardState card_drive_time;
static DashCardState card_drive_throt;
static DashCardState card_drive_map;
static DashCardState card_drive_abs;
static DashCardState card_drive_tc1;
static DashCardState card_drive_tc2;
static DashCardState card_drive_boost;

inline void init_page_drive_sprites() { }

inline void render_page_drive_static() {
    if (current_page != PAGE_DRIVE) return;

    gfx.fillScreen(DASH_BG_GLOBAL);

    // Reset caches for fresh render
    card_drive_fuel.reset();
    card_drive_fuel_toadd.reset();
    card_drive_speed.reset();
    card_drive_last_lap.reset();
    card_drive_fuel_last.reset();
    card_drive_laps_2_go.reset();
    card_drive_gear.reset();
    card_drive_best_lap.reset();
    card_drive_lap.reset();
    card_drive_pos.reset();
    card_drive_bias.reset();
    card_drive_delta.reset();
    card_drive_time.reset();
    card_drive_throt.reset();
    card_drive_map.reset();
    card_drive_abs.reset();
    card_drive_tc1.reset();
    card_drive_tc2.reset();
    card_drive_boost.reset();

    // -------------------------------------------------------------------------
    // GRID STATIC OUTLINES (Following the GRID DDU-5 layout)
    // -------------------------------------------------------------------------
    // Row 1 (Top)
    draw_element_card(1, 1, 1, 2, "FUEL", "0.0", TFT_GREEN, TFT_GREEN);
    draw_element_card(1, 2, 1, 2, "FUEL +", "--", TFT_WHITE);
    draw_element_card(1, 3, 3, 1, "SPEED", "0", TFT_CYAN);

    draw_element_card(1, 8, 4, 1, "LAST", "L-:--.---", TFT_WHITE);
    draw_element_card(2, 8, 4, 1, "BEST", "B-:--.---", TFT_WHITE);
    draw_element_card(3, 8, 4, 1, "TIME", "C0:00.00", TFT_WHITE);
  
    // Row 2
    draw_element_card(3, 1, 1, 2, "FUEL LAST", "-", TFT_WHITE);
   // draw_element_card(3, 4, 3, 1, "LAPS 2 GO", "--", TFT_WHITE);
    draw_element_card(2, 4, 4, 3, "GEAR", "3", TFT_GREEN, TFT_WHITE); // Hero Center Gear Box
     // Row 3
    draw_element_card(5, 1, 1, 2, "LAP", "x", TFT_WHITE);
    draw_element_card(5, 2, 1, 2, "POS", "x", TFT_WHITE);
    draw_element_card(5, 3, 2, 2, "BIAS", "50.0", TFT_WHITE, TFT_RED); // Highlight border
    draw_element_card(5, 6, 2, 2, "DELTA", "-", TFT_WHITE);
   

    // Row 4 (Bottom)
   // draw_element_card(5, 1, 2, 1, "THROT", "0", TFT_MAGENTA);
   // draw_element_card(5, 3, 2, 1, "MAP", "0", TFT_GREEN);
    draw_element_card(5, 5, 1, 2, "ABS", "0", TFT_ORANGE);
    
    // Center Logo Badge Plate
   // draw_logo_card(4, 7, 1, 2);
    draw_element_card(  5, 9, 1, 2, "MAP", "-", TFT_YELLOW);
    draw_element_card(5, 9, 1, 2, "TC 1", "0", TFT_YELLOW);
    draw_element_card(5, 10, 1, 2, "TC 2", "0", TFT_BLUE);
    draw_element_card(5, 11, 1, 2, "Boost", "0", TFT_GREEN, TFT_GREEN);
}

inline void update_drive_page_ui(const SimTelemetry& snap) {
    if (current_page != PAGE_DRIVE) return;

    // Dynamic Gear Color
    uint16_t gear_col = (strcmp(snap.gear, "R") == 0) ? TFT_RED : 
                        ((strcmp(snap.gear, "N") == 0) ? TFT_GREEN : TFT_YELLOW);

    // Format metrics
    char fuel_buf[16], speed_buf[16], lap_buf[16], pos_buf[16], bias_buf[16], delta_buf[16];
    snprintf(fuel_buf, sizeof(fuel_buf), "%.2f", snap.fuel_pct * 100.0f);
    snprintf(speed_buf, sizeof(speed_buf), "%d", (int)snap.speed_kmh);
    snprintf(lap_buf, sizeof(lap_buf), "%d", snap.current_lap);
    snprintf(pos_buf, sizeof(pos_buf), "%d", snap.position);
    snprintf(bias_buf, sizeof(bias_buf), "%.1f", snap.brake_bias);

    if (snap.delta_valid) {
        snprintf(delta_buf, sizeof(delta_buf), "%+.2f", snap.delta_to_best);
    } else {
        snprintf(delta_buf, sizeof(delta_buf), "-");
    }

    // Pipeline updates matching grid positions
    dash_element(PAGE_DRIVE, card_drive_fuel,      1, 1, 1, 2, "FUEL", snap.fuel_pct * 100.0f, 0.0f, 100.0f, 0.1f, "%.2f", TFT_GREEN);
    dash_element(PAGE_DRIVE, card_drive_speed,     1, 3, 3, 1, "SPEED", snap.speed_kmh, 0.0f, 350.0f, 1.0f, "%d", TFT_CYAN);
    dash_element(PAGE_DRIVE, card_drive_last_lap,  1, 8, 3, 1, "LAST", snap.last_lap_time, TFT_WHITE);
    dash_element(PAGE_DRIVE, card_drive_best_lap,  2, 8, 2, 1, "BEST", snap.best_lap_time, TFT_WHITE);  
        dash_element(PAGE_DRIVE, card_drive_best_lap,  2, 8, 2, 1, "BEST", snap.current_lap, TFT_WHITE);  
    dash_element(PAGE_DRIVE, card_drive_lap,       5, 1, 1, 2, "LAP", snap.current_lap, TFT_WHITE);


    dash_element(PAGE_DRIVE, card_drive_gear,      2, 4, 4, 3, "Gear", snap.gear, gear_col);



    dash_element(PAGE_DRIVE, card_drive_pos,       5, 2, 1, 2, "POS", snap.position, TFT_WHITE);
    dash_element(PAGE_DRIVE, card_drive_bias,      5, 3, 2, 2, "BIAS", snap.brake_bias, 0.0f, 100.0f, 0.1f, "%.1f", TFT_WHITE);

    dash_element(PAGE_DRIVE, card_drive_abs,       5, 7, 1, 2, "ABS", snap.abs_level, TFT_ORANGE);
        dash_element(PAGE_DRIVE, card_drive_tc1,   5, 9, 1, 2, "MAP", snap.tc_level, TFT_YELLOW);
    dash_element(PAGE_DRIVE, card_drive_tc1,       5, 9, 1, 2, "TC1", snap.tc_level, TFT_YELLOW);
    dash_element(PAGE_DRIVE, card_drive_tc2,       5, 11, 1, 2, "TC2", snap.tc_cut, TFT_BLUE);


    dash_element(PAGE_DRIVE, card_drive_delta,     5, 6, 2, 2, "DELTA", delta_buf, snap.delta_to_best <= 0 ? TFT_GREEN : TFT_RED);


}