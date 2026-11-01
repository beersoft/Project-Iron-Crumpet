#pragma once
#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "ui_settings.h"
#include "config_manager.h"   // <--- Make sure config manager is included here so AppConfig is defined
#include "simhub_client.h"
#include "dash_helpers.h"
// -----------------------------------------------------------------------------
// Corporate Theme Colors
// -----------------------------------------------------------------------------
#ifndef COL_CORP_NAVY
#define COL_CORP_NAVY 0x0A2B
#endif
#ifndef COL_CORP_RED
#define COL_CORP_RED  0xD800
#endif
#ifndef COL_CORP_BG
#define COL_CORP_BG   0xFFFF
#endif

// -----------------------------------------------------------------------------
// Core Dashboard Pages Enum
// -----------------------------------------------------------------------------
enum DashboardPage {
    PAGE_RACE = 0,
    PAGE_DRIVE,
    PAGE_ELECTRONICS,
    PAGE_TYRES,
    PAGE_TIMING,
    PAGE_STATS,
    PAGE_IDLE,
    PAGE_DIALS,
    TOTAL_PAGES
};

inline const char* get_page_name(DashboardPage page) {
    switch (page) {
        case PAGE_RACE:        return "RACE";
        case PAGE_DRIVE:       return "DRIVE";
        case PAGE_ELECTRONICS: return "ELECTRONICS";
        case PAGE_TYRES:       return "TYRES";
        case PAGE_TIMING:      return "TIMING";
        case PAGE_STATS:       return "STATS";
        case PAGE_IDLE:        return "STANDBY";
        case PAGE_DIALS:       return "DIALS";
        default:               return "DASH";
    }
}

struct SettingAlert {
    bool active           = false;
    char title[32]        = {0};
    char value[32]        = {0};
    uint16_t color        = TFT_WHITE;
    uint32_t expires      = 0;
};

struct DashboardDisplayCache {
    int speed             = 0;
    int rpm               = 0;
    char gear[4]          = "N";
    char curr_lap[24]     = "00:00.00";
    char last_lap_stored[24] = "00:00.000";
    char all_time_best[24]   = "00:00.000";
    char game[32]         = {0};
    bool connected        = false;
    char flag_name[16]    = {0};

    int abs_level         = 0;
    bool abs_active       = false;
    int tc_level          = 0;
    bool tc_active        = false;
    int tc_cut            = 0;
    int tc_slip           = 0;
    int engine_map        = 0;
    float brake_bias      = 0.0f;
    float brake_migration = 0.0f;
    float fuel            = 0.0f;
    float battery_charge_pct = 0.0f;
    float air_temp        = 0.0f;
    int handbrake         = 0;
    int lap_num           = 0;
    int position          = 0;
    bool pit_limiter_on   = false;
    int headlights_on     = 0;
    int highbeam_flash_on = 0;
    int wipers_on         = 0;

    bool spotter_left     = false;
    bool spotter_right    = false;

    uint16_t last_rpm_box_color = 0;
    uint16_t last_fl_col  = 0;
    uint16_t last_fr_col  = 0;
    uint16_t last_rl_col  = 0;
    uint16_t last_rr_col  = 0;

    void reset_to_zero() {
        speed = 0;
        rpm = 0;
        gear[0] = 'N'; gear[1] = '\0';
        curr_lap[0] = '\0';
        last_lap_stored[0] = '\0';
        all_time_best[0] = '\0';
        game[0] = '\0';
        lap_num = 0;
        position = 0;
        abs_level = 0;
        tc_level = 0;
        last_rpm_box_color = 0;
        last_fl_col = 0;
        last_fr_col = 0;
        last_rl_col = 0;
        last_rr_col = 0;
    }

    void invalidate() {
        speed = -999;
        rpm = -999;
        gear[0] = '\0';
        lap_num = -999;
        position = -999;
        curr_lap[0] = '\0';
        last_lap_stored[0] = '\0';
        all_time_best[0] = '\0';
        flag_name[0] = '\0';
        abs_level = -999;
        tc_level = -999;
        last_fl_col = 0;
        last_fr_col = 0;
        last_rl_col = 0;
        last_rr_col = 0;
    }
};

// -----------------------------------------------------------------------------
// Smoothing & Timing Classes
// -----------------------------------------------------------------------------
class TelemetryExtrapolator {
private:
    float current_val = 0.0f;
    float target_val  = 0.0f;
    uint32_t last_update = 0;

public:
    void update_target(float target) {
        target_val = target;
        last_update = millis();
    }

    float get_extrapolated(float min_v, float max_v) {
        float factor = 0.38f;
        current_val += (target_val - current_val) * factor;
        return constrain(current_val, min_v, max_v);
    }
};

class LocalLapTimer {
private:
    uint32_t baseline_lap_ms = 0;
    uint32_t sync_millis = 0;
    int last_lap = -1;
    bool running = false;
    bool has_synced = false;

    uint32_t freeze_until = 0;
    char completed_lap_str[24] = "";

public:
    uint32_t parse_time_str(const char* s) {
        if (!s || strlen(s) == 0) return 0;
        int m = 0, sec = 0, ms = 0;
        if (sscanf(s, "%d:%d.%d", &m, &sec, &ms) == 3) {
            return (m * 60000) + (sec * 1000) + ms;
        }
        float sec_f = atof(s);
        return (uint32_t)(sec_f * 1000.0f);
    }

    void sync(const char* time_str, int lap, bool active, const char* last_lap_str = nullptr) {
        running = active;
        if (!running) {
            baseline_lap_ms = 0;
            has_synced = false;
            return;
        }

        uint32_t packet_ms = parse_time_str(time_str);

        if (last_lap != -1 && (lap > last_lap || (packet_ms < baseline_lap_ms && baseline_lap_ms > 20000))) {
            if (last_lap_str && strlen(last_lap_str) > 0 && strcmp(last_lap_str, "--:--.---") != 0) {
                strncpy(completed_lap_str, last_lap_str, sizeof(completed_lap_str) - 1);
            } else {
                format_ms(completed_lap_str, sizeof(completed_lap_str), get_elapsed_ms());
            }
            freeze_until = millis() + 5000;
        }

        last_lap = lap;
        baseline_lap_ms = packet_ms;
        sync_millis = millis();
        has_synced = true;
    }

    uint32_t get_elapsed_ms() {
        if (!running || !has_synced) return 0;
        return baseline_lap_ms + (millis() - sync_millis);
    }

    static void format_ms(char* dest, size_t max_len, uint32_t elapsed) {
        uint32_t mins = elapsed / 60000;
        uint32_t secs = (elapsed % 60000) / 1000;
        uint32_t ms   = elapsed % 1000;
        snprintf(dest, max_len, "%02u:%02u.%03u", mins, secs, ms);
    }

    void format_current(char* dest, size_t max_len, const char* fallback_str = nullptr) {
        if (millis() < freeze_until && strlen(completed_lap_str) > 0) {
            strncpy(dest, completed_lap_str, max_len - 1);
            dest[max_len - 1] = '\0';
            return;
        }

        if (!running || !has_synced) {
            if (fallback_str && strlen(fallback_str) > 0) {
                strncpy(dest, fallback_str, max_len - 1);
                dest[max_len - 1] = '\0';
            } else {
                snprintf(dest, max_len, "--:--.---");
            }
            return;
        }

        format_ms(dest, max_len, get_elapsed_ms());
    }

    bool is_frozen() const {
        return millis() < freeze_until;
    }
};

struct DashCardState {
    TelemetryExtrapolator smoother;
    float current_val     = 0.0f;
    float delta_threshold = 1.0f;
    char text_cache[32]   = "";
    uint16_t color_cache  = 0xFFFF;
    bool initialized      = false;

    void reset() {
        current_val     = 0.0f;
        text_cache[0]   = '\0';
        color_cache     = 0xFFFF;
        initialized     = false;
    }
};

extern DashboardPage current_page;
extern LGFX gfx;

// Flicker-Free Responsive Card Drawing Primitive
inline void draw_element_card(int row, int col, int col_span, int row_span,
                              const char* label, const char* value_str,
                              uint16_t text_color, uint16_t border_color = DASH_CARD_BORDER) {
    int32_t x, y, w, h;
    grid_to_pixels(row, col, row_span, col_span, x, y, w, h);

    int cell_h = 56;
    int gap = 6;
    int header_row_h = cell_h;

    if (row_span == 1) {
        if (label && strlen(label) > 0) {
            int lbl_w = 64;
            int lbl_h = h - 10;
            
            gfx.fillRoundRect(x + 5, y + 5, lbl_w, lbl_h, 4, 0x18E3);
            gfx.drawRoundRect(x + 5, y + 5, lbl_w, lbl_h, 4, TFT_WHITE);

            gfx.setFont(&Orbitron_Bold_20);
            gfx.setTextSize(0.45f);
            gfx.setTextDatum(MC_DATUM);
            gfx.setTextColor(TFT_WHITE, 0x18E3);
            gfx.drawString(label, x + 5 + (lbl_w / 2), y + (h / 2));
        }

        int clear_x = x + 72;
        int clear_y = y + 3;
        int clear_w = w - 76;
        int clear_h = h - 6;
        gfx.fillRect(clear_x, clear_y, clear_w, clear_h, DASH_CARD_BG);

        if (value_str && strlen(value_str) > 0) {
            gfx.setFont(DASH_FONT_DATA_NUM);
            gfx.setTextSize(DASH_FONT_NUM_SM_SIZE);
            gfx.setTextDatum(MR_DATUM);
            gfx.setTextColor(text_color, DASH_CARD_BG);
            gfx.drawString(value_str, x + w - 12, y + (h / 2));
        }
    } else {
        if (label && strlen(label) > 0) {
            gfx.fillRect(x + 2, y + 2, w - 4, header_row_h, 0x18E3);
            gfx.drawRoundRect(x + 2, y + 2, w - 4, header_row_h, 4, TFT_WHITE);
            gfx.drawFastHLine(x + 2, y + header_row_h + 2, w - 4, border_color);

            gfx.setFont(&Orbitron_Bold_20);
            gfx.setTextSize(0.55f);
            gfx.setTextDatum(MC_DATUM);
            gfx.setTextColor(TFT_WHITE, 0x18E3);
            gfx.drawString(label, x + (w / 2), y + (header_row_h / 2) + 2);
        }

        int content_area_y = y + header_row_h + gap;
        int content_area_h = h - header_row_h - gap;
        
        gfx.fillRect(x + 3, content_area_y, w - 6, content_area_h - 2, DASH_CARD_BG);

        if (value_str && strlen(value_str) > 0) {
            gfx.setFont(DASH_FONT_DATA_NUM);
            gfx.setTextDatum(MC_DATUM);
            gfx.setTextColor(text_color, DASH_CARD_BG);

            int content_center_y = content_area_y + (content_area_h / 2);

            float target_scale;
            if (content_area_h >= 240) {
                target_scale = DASH_FONT_NUM_HERO_SIZE;
            } else if (content_area_h >= 120) {
                target_scale = DASH_FONT_NUM_MD_SIZE;
            } else {
                target_scale = DASH_FONT_NUM_SM_SIZE;
            }

            int max_text_width = w - 16;
            gfx.setTextSize(target_scale);
            int rendered_width = gfx.textWidth(value_str);

            if (rendered_width > max_text_width && rendered_width > 0) {
                float reduction_factor = (float)max_text_width / (float)rendered_width;
                target_scale *= reduction_factor;
                gfx.setTextSize(target_scale);
            }

            gfx.drawString(value_str, x + (w / 2), content_center_y);
        }
    }
}

// Logo Badge Helper
template <typename T>
inline void draw_assist_bar_logo(T* target, int32_t x, int32_t y, int32_t w, int32_t h) {
    target->fillRoundRect(x, y, w, h, 6, TFT_WHITE);
    target->drawRoundRect(x, y, w, h, 6, 0xCE79);

    float scale = (float)h / 260.0f;
    if (scale > 0.35f) scale = 0.35f;

    int emblem_x = x + 12;
    int emblem_y = y + (h / 2) - (int)(105 * scale);

    #define ASX(v) (emblem_x + (int)((v) * scale))
    #define ASY(v) (emblem_y + (int)((v) * scale))

    target->fillCircle(ASX(40),  ASY(42), (int)(18 * scale), COL_CORP_NAVY);
    target->fillCircle(ASX(75),  ASY(32), (int)(24 * scale), COL_CORP_NAVY);
    target->fillCircle(ASX(112), ASY(42), (int)(18 * scale), COL_CORP_NAVY);
    target->fillCircle(ASX(128), ASY(52), (int)(12 * scale), COL_CORP_NAVY);

    target->fillCircle(ASX(40),  ASY(42), (int)(9 * scale),  TFT_WHITE);
    target->fillCircle(ASX(75),  ASY(32), (int)(15 * scale), TFT_WHITE);
    target->fillCircle(ASX(112), ASY(42), (int)(9 * scale),  TFT_WHITE);
    target->fillCircle(ASX(128), ASY(52), (int)(6 * scale),  TFT_WHITE);

    target->fillRect(ASX(14), ASY(62), (int)(128 * scale), (int)(9 * scale), COL_CORP_NAVY);
    target->fillRect(ASX(96), ASY(71), (int)(14 * scale), (int)(32 * scale), COL_CORP_NAVY);
    target->fillCircle(ASX(103), ASY(103), (int)(7 * scale), COL_CORP_NAVY);

    target->fillTriangle(ASX(28), ASY(78), ASX(128), ASY(78), ASX(38), ASY(190), COL_CORP_RED);
    target->fillTriangle(ASX(128), ASY(78), ASX(116), ASY(190), ASX(38), ASY(190), COL_CORP_RED);

    target->fillRect(ASX(39), ASY(78), (int)(10 * scale), (int)(112 * scale), TFT_WHITE);
    target->fillRect(ASX(56), ASY(96), (int)(10 * scale), (int)(42 * scale), TFT_WHITE);
    target->fillRect(ASX(56), ASY(140), (int)(10 * scale), (int)(14 * scale), TFT_WHITE);

    for (int t = 0; t < (int)(10 * scale); t++) {
        target->drawLine(ASX(20 + t), ASY(71), ASX(32 + t), ASY(205), COL_CORP_NAVY);
    }
    for (int t = 0; t < (int)(8 * scale); t++) {
        target->drawLine(ASX(134 + t), ASY(71), ASX(120 + t), ASY(195), COL_CORP_NAVY);
    }

    target->fillRect(ASX(32), ASY(195), (int)(58 * scale), (int)(12 * scale), COL_CORP_NAVY);
    target->fillRect(ASX(98), ASY(195), (int)(30 * scale), (int)(12 * scale), COL_CORP_RED);

    #undef ASX
    #undef ASY

    int text_x = emblem_x + (int)(150 * scale) + 6;
    target->setTextDatum(TL_DATUM);

    target->setFont(&fonts::Font2);
    target->setTextColor(COL_CORP_NAVY, TFT_WHITE);
    target->drawString("BEERSOFT", text_x, y + (h / 2) - 14);

    target->setFont(&fonts::Font0);
    target->setTextColor(COL_CORP_RED, TFT_WHITE);
    target->drawString("INTERACTIVE", text_x, y + (h / 2) + 2);
}

inline void draw_logo_card(int row, int col, int row_span, int col_span) {
    int32_t x, y, w, h;
    grid_to_pixels(row, col, row_span, col_span, x, y, w, h);
    draw_assist_bar_logo(&gfx, x, y, w, h);
}

// -----------------------------------------------------------------------------
// dash_element Dispatchers
// -----------------------------------------------------------------------------
inline void dash_element(DashboardPage target_page, DashCardState& card,
                         int row, int col, int col_span, int row_span,
                         const char* label, float raw_target, float min_clamp, float max_clamp,
                         float delta_thresh, const char* format, uint16_t text_color) {
    if (current_page != target_page) return;

    card.smoother.update_target(raw_target);
    float smoothed = card.smoother.get_extrapolated(min_clamp, max_clamp);
    float delta = fabs(smoothed - card.current_val);

    if (!card.initialized || delta >= delta_thresh || text_color != card.color_cache) {
        card.current_val = smoothed;
        card.color_cache = text_color;
        card.initialized = true;

        char buf[32];
        if (strchr(format, 'd') != nullptr) {
            snprintf(buf, sizeof(buf), format, (int)smoothed);
        } else {
            snprintf(buf, sizeof(buf), format, smoothed);
        }
        strncpy(card.text_cache, buf, sizeof(card.text_cache) - 1);
        card.text_cache[sizeof(card.text_cache) - 1] = '\0';

        draw_element_card(row, col, col_span, row_span, label, buf, text_color);
    }
}

inline void dash_element(DashboardPage target_page, DashCardState& card,
                         int row, int col, int col_span, int row_span,
                         const char* label, int raw_val, uint16_t text_color) {
    if (current_page != target_page) return;

    if (!card.initialized || raw_val != (int)card.current_val || text_color != card.color_cache) {
        card.current_val = (float)raw_val;
        card.color_cache = text_color;
        card.initialized = true;

        char buf[16];
        snprintf(buf, sizeof(buf), "%d", raw_val);
        strncpy(card.text_cache, buf, sizeof(card.text_cache) - 1);
        card.text_cache[sizeof(card.text_cache) - 1] = '\0';

        draw_element_card(row, col, col_span, row_span, label, buf, text_color);
    }
}

inline void dash_element(DashboardPage target_page, DashCardState& card,
                         int row, int col, int col_span, int row_span,
                         const char* label, const char* str_val, uint16_t text_color) {
    if (current_page != target_page) return;

    if (!card.initialized || strcmp(str_val, card.text_cache) != 0 || text_color != card.color_cache) {
        strncpy(card.text_cache, str_val, sizeof(card.text_cache) - 1);
        card.text_cache[sizeof(card.text_cache) - 1] = '\0';
        card.color_cache = text_color;
        card.initialized = true;

        draw_element_card(row, col, col_span, row_span, label, str_val, text_color);
    }
}

// -----------------------------------------------------------------------------
// Test & Demo Sweep Generator Mode
// -----------------------------------------------------------------------------
inline bool demo_test_mode = false;
inline void generate_demo_telemetry(SimTelemetry& telem) {
    static uint32_t demo_start_ms = 0;
    uint32_t now = millis();
    if (demo_start_ms == 0) demo_start_ms = now;

    uint32_t elapsed = (now - demo_start_ms) % 6000;
    float progress;
    if (elapsed < 3000) {
        progress = (float)elapsed / 3000.0f;
    } else {
        progress = 1.0f - ((float)(elapsed - 3000) / 3000.0f);
    }

    telem.connected    = true;
    telem.game_running = true;
    telem.in_race      = true;
    telem.max_rpm      = 8000;

    telem.speed_kmh = progress * 300.0f;
    telem.rpm       = (int)(progress * 8000.0f);

    int gear_idx = (int)(progress * 6.99f);
    if (gear_idx == 0) {
        strncpy(telem.gear, "N", sizeof(telem.gear));
    } else {
        snprintf(telem.gear, sizeof(telem.gear), "%d", gear_idx);
    }
    strncpy(telem.track_name, "SUZUKA GP", sizeof(telem.track_name));
    bool flash_fast = ((elapsed / 250) % 2 == 0);
    bool flash_med  = ((elapsed / 500) % 2 == 0);
    telem.abs_available  = true;
    telem.abs_active     = flash_med;
    telem.tc_available   = true;
    telem.tc_active      = !flash_med;
    telem.pit_limiter_on = !flash_med;

    telem.headlights_on     = !flash_med;
    telem.highbeam_flash_on = flash_fast;

    telem.wipers_available  = true;
    telem.wipers_on         = flash_med ? 1 : 0;

    telem.ignition_available = flash_med;
    telem.engine_running     = flash_med;

    telem.spotter_left  = flash_med;
    telem.spotter_right = !flash_med;

    if (progress < 0.25f)       strncpy(telem.flag_name, "Green", sizeof(telem.flag_name));
    else if (progress < 0.50f)  strncpy(telem.flag_name, "Yellow", sizeof(telem.flag_name));
    else if (progress < 0.75f)  strncpy(telem.flag_name, "Blue", sizeof(telem.flag_name));
    else                        strncpy(telem.flag_name, "Chequered", sizeof(telem.flag_name));

    telem.current_lap = 5;
    telem.position    = 1;
    telem.sector_1_status = 2;
    telem.sector_2_status = 3;
    telem.sector_3_status = 1;
    snprintf(telem.sector_1, sizeof(telem.sector_1), "28.412");
    snprintf(telem.sector_2, sizeof(telem.sector_2), "31.055");
    snprintf(telem.sector_3, sizeof(telem.sector_3), "24.180");
    snprintf(telem.current_lap_time, sizeof(telem.current_lap_time), "01:23.647");
    snprintf(telem.last_lap_time,    sizeof(telem.last_lap_time),    "01:24.102");
    snprintf(telem.best_lap_time,    sizeof(telem.best_lap_time),    "01:23.210");

    float sim_temp = 50.0f + (progress * 80.0f);
    float sim_brk  = 150.0f + (progress * 700.0f);

    telem.fl.temp_c = sim_temp; telem.fl.brake_c = sim_brk; telem.fl.pressure = 27.5f;
    telem.fr.temp_c = sim_temp; telem.fr.brake_c = sim_brk; telem.fr.pressure = 27.5f;
    telem.rl.temp_c = sim_temp; telem.rl.brake_c = sim_brk; telem.rl.pressure = 26.8f;
    telem.rr.temp_c = sim_temp; telem.rr.brake_c = sim_brk; telem.rr.pressure = 26.8f;
}

// -----------------------------------------------------------------------------
// Quick Page Menu Overlay State (8 Items)
// -----------------------------------------------------------------------------
inline bool page_menu_open = false;

struct PageMenuItem {
    DashboardPage page;
    const char* label;
    int32_t x, y, w, h;
};

inline PageMenuItem page_menu_items[8] = {
    { PAGE_RACE,        "1. RACE",        0, 0, 0, 0 },
    { PAGE_DIALS,       "2. DIALS",       0, 0, 0, 0 },
    { PAGE_DRIVE,       "3. DRIVE",       0, 0, 0, 0 },
    { PAGE_ELECTRONICS, "4. ELECTRONICS", 0, 0, 0, 0 },
    { PAGE_TYRES,       "5. TYRES",       0, 0, 0, 0 },
    { PAGE_TIMING,      "6. TIMING",      0, 0, 0, 0 },
    { PAGE_STATS,       "7. BOOT",        0, 0, 0, 0 },
    { PAGE_IDLE,        "8. STATUS",      0, 0, 0, 0 }
};

inline void render_page_menu(int highlight_idx = -1) {
    const int menu_w = 520;
    const int menu_h = 340;
    const int menu_x = (800 - menu_w) / 2;
    const int menu_y = (480 - menu_h) / 2;

    gfx.fillRoundRect(menu_x, menu_y, menu_w, menu_h, 12, DASH_CARD_BG);
    gfx.drawRoundRect(menu_x, menu_y, menu_w, menu_h, 12, DASH_CARD_BORDER);
    gfx.drawRoundRect(menu_x + 1, menu_y + 1, menu_w - 2, menu_h - 2, 12, DASH_CARD_BORDER);

    gfx.setFont(&fonts::Font4);
    gfx.setTextSize(0.85f);
    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(DASH_TEXT_SLATE, DASH_CARD_BG);
    gfx.drawString("SELECT DASHBOARD VIEW", menu_x + (menu_w / 2), menu_y + 24);

    int btn_w = 230;
    int btn_h = 44;
    int gap_x = 18;
    int gap_y = 10;
    int start_x = menu_x + 21;
    int start_y = menu_y + 48;

    for (int i = 0; i < 8; i++) {
        int col = i % 2;
        int row = i / 2;

        int bx = start_x + (col * (btn_w + gap_x));
        int by = start_y + (row * (btn_h + gap_y));

        page_menu_items[i].x = bx;
        page_menu_items[i].y = by;
        page_menu_items[i].w = btn_w;
        page_menu_items[i].h = btn_h;

        bool is_selected = (page_menu_items[i].page == current_page);
        bool is_pressed  = (highlight_idx == i);

        uint16_t bg_col     = is_pressed ? TFT_CYAN : (is_selected ? 0x1A4F : DASH_BG_GLOBAL);
        uint16_t border_col = is_pressed ? TFT_WHITE : (is_selected ? TFT_CYAN : DASH_CARD_BORDER);
        uint16_t text_col   = is_pressed ? TFT_BLACK : (is_selected ? TFT_CYAN : TFT_WHITE);

        gfx.fillRoundRect(bx, by, btn_w, btn_h, 8, bg_col);
        gfx.drawRoundRect(bx, by, btn_w, btn_h, 8, border_col);

        gfx.setFont(&fonts::Font4);
        gfx.setTextSize(0.85f);
        gfx.setTextDatum(MC_DATUM);
        gfx.setTextColor(text_col, bg_col);
        gfx.drawString(page_menu_items[i].label, bx + (btn_w / 2), by + (btn_h / 2));
    }
}