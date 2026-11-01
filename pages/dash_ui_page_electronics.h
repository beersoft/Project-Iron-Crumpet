#pragma once

static DashCardState card_bbias;
static DashCardState card_bmig;
static DashCardState card_abs;
static DashCardState card_tc_main;
static DashCardState card_tc_cut;
static DashCardState card_tc_slip;

inline void init_page_electronics_sprites() { }

inline void render_page_electronics_static() {
    if (current_page != PAGE_ELECTRONICS) return;
    gfx.fillScreen(DASH_BG_GLOBAL);

    card_bbias.reset();
    card_bmig.reset();
    card_abs.reset();
    card_tc_main.reset();
    card_tc_cut.reset();
    card_tc_slip.reset();

    // Baseline placeholders (Row, Col, Col_Span, Row_Span)
    draw_element_card(1, 1, 4, 3, "BRAKE BIAS",      "--.- %", TFT_CYAN);
    draw_element_card(1, 5, 4, 3, "BRAKE MIGRATION", "+0.0",   TFT_CYAN);
    draw_element_card(1, 9, 4, 3, "ABS SYSTEM",      "-",      TFT_YELLOW);

    draw_element_card(4, 1, 4, 3, "TC MAIN",         "-",      TFT_BLUE);
    draw_element_card(4, 5, 4, 3, "TC CUT",          "-",      TFT_BLUE);
    draw_element_card(4, 9, 4, 3, "TC SLIP",         "-",      TFT_BLUE);
}

inline void update_electronics_page_ui(const SimTelemetry& snap) {
    if (current_page != PAGE_ELECTRONICS) return;

    // Brake Bias (0.1% delta threshold)
    dash_element(PAGE_ELECTRONICS, card_bbias, 1, 1, 4, 3, 
                 "BRAKE BIAS", snap.brake_bias, 0.0f, 100.0f, 0.1f, "%.1f %%", TFT_WHITE);

    // Brake Migration (0.1 step threshold)
    dash_element(PAGE_ELECTRONICS, card_bmig,  1, 5, 4, 3, 
                 "BRAKE MIGRATION", snap.brake_migration, -10.0f, 10.0f, 0.1f, "%+.1f", TFT_WHITE);

    // ABS System (Dynamic active yellow highlight)
    uint16_t abs_col = snap.abs_active ? TFT_YELLOW : TFT_WHITE;
    dash_element(PAGE_ELECTRONICS, card_abs, 1, 9, 4, 3, 
                 "ABS SYSTEM", snap.abs_level, abs_col);

    // TC Main (Dynamic active blue highlight)
    uint16_t tc_col = snap.tc_active ? TFT_BLUE : TFT_WHITE;
    dash_element(PAGE_ELECTRONICS, card_tc_main, 4, 1, 4, 3, 
                 "TC MAIN", snap.tc_level, tc_col);

    // TC Cut
    dash_element(PAGE_ELECTRONICS, card_tc_cut,  4, 5, 4, 3, 
                 "TC CUT", snap.tc_cut, TFT_WHITE);

    // TC Slip
    dash_element(PAGE_ELECTRONICS, card_tc_slip, 4, 9, 4, 3, 
                 "TC SLIP", snap.tc_slip, TFT_WHITE);
}