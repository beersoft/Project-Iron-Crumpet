#pragma once

struct TimingLapHistory {
    char times[5][24] = { "--:--.---", "--:--.---", "--:--.---", "--:--.---", "--:--.---" };
    int lap_numbers[5] = {0, 0, 0, 0, 0};
    int count = 0;
};

inline TimingLapHistory timing_history;
inline char last_recorded_lap_str[24] = {0};
inline int last_recorded_lap_num = -1;

inline void push_lap_history(const char* lap_time_str, int completed_lap_num) {
    if (!lap_time_str || strlen(lap_time_str) == 0 || strcmp(lap_time_str, "--:--.---") == 0) return;

    for (int i = 4; i > 0; i--) {
        strncpy(timing_history.times[i], timing_history.times[i - 1], sizeof(timing_history.times[i]) - 1);
        timing_history.times[i][sizeof(timing_history.times[i]) - 1] = '\0';
        timing_history.lap_numbers[i] = timing_history.lap_numbers[i - 1];
    }
    strncpy(timing_history.times[0], lap_time_str, sizeof(timing_history.times[0]) - 1);
    timing_history.times[0][sizeof(timing_history.times[0]) - 1] = '\0';
    timing_history.lap_numbers[0] = completed_lap_num;
    if (timing_history.count < 5) timing_history.count++;
}

inline void init_page_timing_sprites() { }

// Card state tracking for the 5 individual lap history cards
static DashCardState card_history_laps[5];

inline void render_lap_history_cards() {
    for (int i = 0; i < 5; i++) {
        char tag[16];
        if (timing_history.lap_numbers[i] > 0) {
            snprintf(tag, sizeof(tag), "LAP %d", timing_history.lap_numbers[i]);
        } else {
            snprintf(tag, sizeof(tag), "LAP -%d", i + 1);
        }

        // Each lap gets a 1-row high, 6-column wide card stacked from row 2 to 6 in columns 7-12
        uint16_t time_col = (i == 0) ? TFT_YELLOW : TFT_WHITE;
        draw_element_card(2 + i, 7, 6, 1, tag, timing_history.times[i], time_col);
    }
}

static DashCardState card_s1;
static DashCardState card_s2;
static DashCardState card_s3;
static DashCardState card_timing_curr_lap;
static DashCardState card_last_lap;
static DashCardState card_best_lap;

inline void render_page_timing_static() {
    if (current_page != PAGE_TIMING) return;
    gfx.fillScreen(DASH_BG_GLOBAL);

    card_s1.reset();
    card_s2.reset();
    card_s3.reset();
    card_timing_curr_lap.reset();
    card_last_lap.reset();
    card_best_lap.reset();
    for (int i = 0; i < 5; i++) {
        card_history_laps[i].reset();
    }

    // Row 1: Sectors (Cols 1-6)
    draw_element_card(1, 1, 3, 1, "S1", "--.---", TFT_WHITE);
    draw_element_card(1, 4, 3, 1, "S2", "--.---", TFT_WHITE);
    draw_element_card(1, 7, 3, 1, "S3", "--.---", TFT_WHITE);

    // Rows 2-5: Main Current/Last/Best Laps (Cols 1-6)
    draw_element_card(2, 1, 6, 1, "CURRENT", "--:--.---", TFT_GREEN);
    draw_element_card(3, 1, 6, 1, "LAST",    "--:--.---", TFT_WHITE);
    draw_element_card(4, 1, 6, 1, "BEST",    "--:--.---", TFT_WHITE);

    // Render individual history cards block on the right (Cols 7-12)
    // First draw a section header background/border or let individual cards handle it:
    render_lap_history_cards();
}

inline void update_timing_page_ui(const SimTelemetry& snap) {
    if (current_page != PAGE_TIMING) return;

    if (strlen(snap.last_lap_time) > 0 && strcmp(snap.last_lap_time, "--:--.---") != 0) {
        if (strcmp(snap.last_lap_time, last_recorded_lap_str) != 0 || 
            (snap.current_lap > 1 && snap.current_lap - 1 != last_recorded_lap_num)) {
            
            strncpy(last_recorded_lap_str, snap.last_lap_time, sizeof(last_recorded_lap_str) - 1);
            last_recorded_lap_num = (snap.current_lap > 1) ? (snap.current_lap - 1) : 1;
            push_lap_history(snap.last_lap_time, last_recorded_lap_num);
            render_lap_history_cards();
        }
    }

    // Sectors
    uint16_t c1 = get_sector_color(snap.sector_1_status);
    const char* disp_s1 = (strlen(snap.sector_1) > 0) ? snap.sector_1 : "--.---";
    dash_element(PAGE_TIMING, card_s1, 1, 1, 3, 1, "S1", disp_s1, c1);

    uint16_t c2 = get_sector_color(snap.sector_2_status);
    const char* disp_s2 = (strlen(snap.sector_2) > 0) ? snap.sector_2 : "--.---";
    dash_element(PAGE_TIMING, card_s2, 1, 4, 3, 1, "S2", disp_s2, c2);

    uint16_t c3 = get_sector_color(snap.sector_3_status);
    const char* disp_s3 = (strlen(snap.sector_3) > 0) ? snap.sector_3 : "--.---";
    dash_element(PAGE_TIMING, card_s3, 1,7, 3, 1, "S3", disp_s3, c3);

    // Main Lap Times
    char cur_lap[24];
    local_lap_clock.format_current(cur_lap, sizeof(cur_lap), snap.current_lap_time);
    dash_element(PAGE_TIMING, card_timing_curr_lap, 2, 1, 6, 1, "CURRENT", cur_lap, TFT_GREEN);

    dash_element(PAGE_TIMING, card_last_lap, 3, 1, 6, 1, "LAST", 
                 (strlen(snap.last_lap_time) > 0) ? snap.last_lap_time : "--:--.---", TFT_WHITE);

    dash_element(PAGE_TIMING, card_best_lap, 4, 1, 6, 1, "BEST", 
                 (strlen(snap.best_lap_time) > 0) ? snap.best_lap_time : "--:--.---", TFT_WHITE);

    // Individual History Card Updates with delta caching
    for (int i = 0; i < 5; i++) {
        char tag[16];
        if (timing_history.lap_numbers[i] > 0) {
            snprintf(tag, sizeof(tag), "LAP %d", timing_history.lap_numbers[i]);
        } else {
            snprintf(tag, sizeof(tag), "LAP -%d", i + 1);
        }
        uint16_t time_col = (i == 0) ? TFT_YELLOW : TFT_WHITE;
        
        // Dispatches through your inline element pipeline safely
        dash_element(PAGE_TIMING, card_history_laps[i], 2 + i, 7, 6, 1, tag, timing_history.times[i], time_col);
    }
}