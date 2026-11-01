// dash_ui.h
#pragma once

#include "dash_ui_common.h"
#include <time.h>

// -----------------------------------------------------------------------------
// Global UI State Definitions
// -----------------------------------------------------------------------------
DashboardPage current_page = PAGE_RACE;
DashboardDisplayCache ui_cache;
SettingAlert active_alert;

TelemetryExtrapolator smooth_speed;
TelemetryExtrapolator smooth_rpm;
TelemetryExtrapolator smooth_delta;
LocalLapTimer local_lap_clock;

LGFX_Sprite* sprFlag     = nullptr; // 130x26 (Left 2 cols)
LGFX_Sprite* sprPit      = nullptr; // 538x26 (Center 8 cols)
LGFX_Sprite* sprTrack    = nullptr; // 130x26 (Right 2 cols)
LGFX_Sprite* sprAssists  = nullptr;
LGFX_Sprite* sprPopup    = nullptr;
LGFX_Sprite* sprSpotterL = nullptr;
LGFX_Sprite* sprSpotterR = nullptr;

// -----------------------------------------------------------------------------
// Sub-Page & Component Inclusions
// -----------------------------------------------------------------------------
#include "pages/dash_ui_alerts.h"
#include "pages/dash_ui_assists.h"
#include "pages/dash_ui_page_race.h"
#include "pages/dash_ui_page_electronics.h"
#include "pages/dash_ui_page_tyres.h"
#include "pages/dash_ui_page_timing.h"
#include "pages/dash_ui_page_stats.h"
#include "pages/dash_ui_page_idle.h"
#include "pages/dash_ui_page_dials.h"
#include "pages/dash_ui_page_drive.h"

// -----------------------------------------------------------------------------
// Dashboard Lifecycle Implementations
// -----------------------------------------------------------------------------
inline void render_active_page_static() {
    gfx.fillScreen(DASH_BG_GLOBAL);

    switch (current_page) {
        case PAGE_RACE:        render_page_race_static();        break;
        case PAGE_DRIVE:       render_page_drive_static();       break;
        case PAGE_ELECTRONICS: render_page_electronics_static(); break;
        case PAGE_TYRES:       render_page_tyres_static();       break;
        case PAGE_TIMING:      render_page_timing_static();      break;
        case PAGE_STATS:       render_page_stats_static();       break;
        case PAGE_IDLE:        render_page_idle_static();        break;
        case PAGE_DIALS:       render_page_dials_static();       break;
        default: break;
    }
}

inline void switch_dashboard_page(DashboardPage target_page) {
    page_menu_open = false;
    demo_test_mode = false;
    current_page   = target_page;

    ui_cache.speed = -1;
    ui_cache.rpm   = -1;
    ui_cache.gear[0] = '\0';
    ui_cache.curr_lap[0] = '\0';
    ui_cache.last_rpm_box_color = 0;
    ui_cache.last_fl_col = 0;
    ui_cache.last_fr_col = 0;
    ui_cache.last_rl_col = 0;
    ui_cache.last_rr_col = 0;

    render_active_page_static();
}

inline void init_dashboard_ui() {
    create_psram_sprite(sprFlag,     130, 26);
    create_psram_sprite(sprPit,      538, 26);
    create_psram_sprite(sprTrack,    130, 26);
    create_psram_sprite(sprAssists,  768, 76);
    create_psram_sprite(sprSpotterL, 12, 195);
    create_psram_sprite(sprSpotterR, 12, 195);

    init_alerts_ui();
    init_page_race_sprites();
    init_page_drive_sprites();
    init_page_electronics_sprites();
    init_page_tyres_sprites();
    init_page_timing_sprites();
    init_page_stats_sprites();
    init_page_idle_sprites();
    init_page_dials_sprites();

    render_active_page_static();
}

inline void update_dashboard_ui() {
    if (page_menu_open) return;
    if (sim_data_mutex == NULL) return;

    SimTelemetry snap;
    if (demo_test_mode) {
        generate_demo_telemetry(snap);
    } else {
        if (xSemaphoreTake(sim_data_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            snap = live_sim_data;
            xSemaphoreGive(sim_data_mutex);
        } else {
            return;
        }
    }

    local_lap_clock.sync(snap.current_lap_time, snap.current_lap, 
                         snap.connected && (snap.game_running || snap.in_race), 
                         snap.last_lap_time);

    uint32_t now = millis();

    // Corner Heartbeat Pixel
    static uint32_t last_hb_toggle = 0;
    static bool hb_state = false;
    if (snap.connected) {
        if (now - last_hb_toggle > 120) {
            last_hb_toggle = now;
            hb_state = !hb_state;
            const uint16_t COLOR_PINK  = 0xF81F;
            const uint16_t COLOR_BROWN = 0x9A60;
            gfx.fillRect(0, 0, 4, 4, hb_state ? COLOR_PINK : COLOR_BROWN);
        }
    } else {
        gfx.fillRect(0, 0, 4, 4, DASH_BG_GLOBAL);
    }

    // Overlay Alert Checking
    if (active_alert.active) {
        if (now > active_alert.expires) {
            active_alert.active = false;
            render_active_page_static();
            ui_cache.speed = -1;
            ui_cache.rpm   = -1;
            ui_cache.gear[0] = '\0';
            ui_cache.curr_lap[0] = '\0';
        } else {
            return;
        }
    }

    // Smooth extrapolators
    smooth_speed.update_target(snap.speed_kmh);
    smooth_rpm.update_target((float)snap.rpm);
    if (snap.delta_valid) {
        smooth_delta.update_target(snap.delta_to_best);
    }

    if (!demo_test_mode) {
        process_driver_alerts(snap);
        if (active_alert.active) return;
    }

    // Cache sync
    if (ui_cache.brake_bias <= 0.0f)     ui_cache.brake_bias      = snap.brake_bias;
    if (ui_cache.brake_migration < 0.0f)  ui_cache.brake_migration = snap.brake_migration;
    if (ui_cache.tc_level < 0)           ui_cache.tc_level        = snap.tc_level;
    if (ui_cache.tc_cut < 0)             ui_cache.tc_cut          = snap.tc_cut;
    if (ui_cache.tc_slip < 0)            ui_cache.tc_slip         = snap.tc_slip;
    if (ui_cache.abs_level < 0)          ui_cache.abs_level       = snap.abs_level;
    if (ui_cache.engine_map < 0)         ui_cache.engine_map      = snap.engine_map;
    ui_cache.lap_num = snap.current_lap;

    bool is_racing = snap.connected && (snap.game_running || snap.in_race) && (snap.rpm > 500);

    if (is_racing && current_page == PAGE_IDLE && !active_alert.active) {
    //    switch_dashboard_page(PAGE_RACE);
        return;
    }

    if (current_page == PAGE_IDLE) {
      //  update_idle_page_ui(snap);
    }

    // -------------------------------------------------------------------------
    // TOP LEFT: Flag Status or Active Page Title
    // -------------------------------------------------------------------------
    static DashboardPage last_rendered_flag_page = (DashboardPage)-1;
    bool flag_changed = (strcmp(snap.flag_name, ui_cache.flag_name) != 0);
    bool page_changed = (current_page != last_rendered_flag_page);

    if (sprFlag && (flag_changed || page_changed)) {
        last_rendered_flag_page = current_page;
        sprFlag->fillSprite(DASH_BG_GLOBAL);

        if (strcasecmp(snap.flag_name, "None") != 0 && strlen(snap.flag_name) > 0) {
            uint16_t bg = DASH_BG_GLOBAL;
            uint16_t fg = TFT_WHITE;

            if (strcasecmp(snap.flag_name, "Yellow") == 0)         { bg = TFT_YELLOW; fg = TFT_BLACK; }
            else if (strcasecmp(snap.flag_name, "Green") == 0)     { bg = TFT_GREEN;  fg = TFT_BLACK; }
            else if (strcasecmp(snap.flag_name, "Blue") == 0)      { bg = TFT_BLUE;   fg = TFT_WHITE; }
            else if (strcasecmp(snap.flag_name, "White") == 0)     { bg = TFT_WHITE;  fg = TFT_BLACK; }
            else if (strcasecmp(snap.flag_name, "Chequered") == 0) { bg = 0x2124;     fg = TFT_WHITE; }

            char flg_str[24];
            snprintf(flg_str, sizeof(flg_str), "%s", snap.flag_name);
            draw_indicator_pill(sprFlag, 1, 1, 128, 24, flg_str, bg, fg, bg, 0.45f);
        } else {
            sprFlag->drawRoundRect(1, 1, 128, 24, 6, DASH_CARD_BORDER);
            sprFlag->setFont(DASH_FONT_LABEL);
            sprFlag->setTextDatum(MC_DATUM);
            sprFlag->setTextSize(0.48f);
            sprFlag->setTextColor(DASH_TEXT_SLATE, DASH_BG_GLOBAL);
            sprFlag->drawString(get_page_name(current_page), 65, 13);
        }
        sprFlag->pushSprite(16, 6);
    }

    // -------------------------------------------------------------------------
    // TOP CENTER: Pit Limiter or Smooth RPM Bar
    // -------------------------------------------------------------------------
    const int rpm_bar_x = 152;
    const int rpm_bar_w = 538;
    const int rpm_inner_w = rpm_bar_w - 4;

    if (snap.pit_limiter_on) {
        if (sprPit) {
            bool flash_on = (now / 200) % 2 == 0;
            sprPit->fillSprite(flash_on ? TFT_BLUE : DASH_BG_GLOBAL);
            sprPit->drawRoundRect(0, 0, rpm_bar_w, 26, 6, TFT_CYAN);
            sprPit->setTextDatum(MC_DATUM);
            sprPit->setFont(DASH_FONT_DATA);
            sprPit->setTextSize(0.55f);
            sprPit->setTextColor(flash_on ? TFT_WHITE : TFT_BLUE, flash_on ? TFT_BLUE : DASH_BG_GLOBAL);
            sprPit->drawString(">>>  PIT LIMITER ACTIVE  <<<", rpm_bar_w / 2, 13);
            sprPit->pushSprite(rpm_bar_x, 6);
        }
    } else {
        float disp_rpm = smooth_rpm.get_extrapolated(0.0f, 20000.0f);
        int max_r = snap.max_rpm > 0 ? snap.max_rpm : 8000;
        int fill_w = map(constrain((int)disp_rpm, 0, max_r), 0, max_r, 0, rpm_inner_w);
        uint16_t bar_color = (disp_rpm > (max_r * 0.92f)) ? TFT_RED : ((disp_rpm > (max_r * 0.78f)) ? TFT_YELLOW : TFT_GREEN);

        gfx.fillRect(rpm_bar_x + 2, 8, fill_w, 22, bar_color);
        if (fill_w < rpm_inner_w) {
            gfx.fillRect(rpm_bar_x + 2 + fill_w, 8, rpm_inner_w - fill_w, 22, DASH_BG_GLOBAL);
        }
        gfx.drawRoundRect(rpm_bar_x, 6, rpm_bar_w, 26, 6, DASH_CARD_BORDER);
    }

    // -------------------------------------------------------------------------
    // TOP RIGHT: Track / Circuit Name
    // -------------------------------------------------------------------------
    static char last_rendered_track[32] = "\x01";
    const char* cur_track = (strlen(snap.track_name) > 0) ? snap.track_name : 
                            ((strlen(snap.game_name) > 0) ? snap.game_name : "TRACK");

    if (sprTrack && (strcmp(cur_track, last_rendered_track) != 0 || page_changed)) {
        strncpy(last_rendered_track, cur_track, sizeof(last_rendered_track) - 1);
        sprTrack->fillSprite(DASH_BG_GLOBAL);
        sprTrack->drawRoundRect(1, 1, 128, 24, 6, DASH_CARD_BORDER);

        sprTrack->setFont(DASH_FONT_LABEL);
        sprTrack->setTextDatum(MC_DATUM);

        float t_scale = 0.45f;
        sprTrack->setTextSize(t_scale);
        int t_width = sprTrack->textWidth(cur_track);
        if (t_width > 116 && t_width > 0) {
            t_scale *= (116.0f / (float)t_width);
            sprTrack->setTextSize(t_scale);
        }

        sprTrack->setTextColor(DASH_TEXT_SLATE, DASH_BG_GLOBAL);
        sprTrack->drawString(cur_track, 65, 13);
        sprTrack->pushSprite(696, 6);
    }

    // -------------------------------------------------------------------------
    // BOTTOM TRAY ASSISTS BAR
    //#applies to active racing pages including drive
    // -------------------------------------------------------------------------
    if (current_page == PAGE_RACE || current_page == PAGE_ELECTRONICS || 
        current_page == PAGE_TYRES || current_page == PAGE_TIMING || 
        current_page == PAGE_STATS || current_page == PAGE_DIALS || current_page == PAGE_DRIVE) {
        static uint32_t last_assist_push = 0;
        if (now - last_assist_push > 66) {
            last_assist_push = now;
            update_assists_tray_ui(snap);
        }
    }

    // -------------------------------------------------------------------------
    // SPOTTER RADAR BARS
    // -------------------------------------------------------------------------
    if (current_page != PAGE_STATS) {
        if (sprSpotterL && snap.spotter_left != ui_cache.spotter_left) {
            ui_cache.spotter_left = snap.spotter_left;
            sprSpotterL->fillSprite(DASH_BG_GLOBAL);
            if (snap.spotter_left) {
                for (int i = 0; i < 4; i++) {
                    int y_off = 10 + (i * 46);
                    sprSpotterL->fillRoundRect(2, y_off, 8, 36, 4, TFT_ORANGE);
                    sprSpotterL->fillTriangle(2, y_off + 18, 10, y_off + 8, 10, y_off + 28, TFT_RED);
                }
            }
            sprSpotterL->pushSprite(2, 85);
        }

        if (sprSpotterR && snap.spotter_right != ui_cache.spotter_right) {
            ui_cache.spotter_right = snap.spotter_right;
            sprSpotterR->fillSprite(DASH_BG_GLOBAL);
            if (snap.spotter_right) {
                for (int i = 0; i < 4; i++) {
                    int y_off = 10 + (i * 46);
                    sprSpotterR->fillRoundRect(2, y_off, 8, 36, 4, TFT_ORANGE);
                    sprSpotterR->fillTriangle(10, y_off + 18, 2, y_off + 8, 2, y_off + 28, TFT_RED);
                }
            }
            sprSpotterR->pushSprite(786, 85);
        }
    }

    // -------------------------------------------------------------------------
    // Sub-Page Dynamic Updates Dispatcher
    // -------------------------------------------------------------------------
    switch (current_page) {
        case PAGE_RACE:        update_race_page_ui(snap);        break;
        case PAGE_DRIVE:       update_drive_page_ui(snap);       break;
        case PAGE_ELECTRONICS: update_electronics_page_ui(snap); break;
        case PAGE_TYRES:       update_tyres_page_ui(snap);       break;
        case PAGE_TIMING:      update_timing_page_ui(snap);      break;
        case PAGE_STATS:       update_stats_page_ui(snap);       break;
        case PAGE_DIALS:       update_dials_page_ui(snap);       break;
        default: break;
    }

    strncpy(ui_cache.flag_name, snap.flag_name, sizeof(ui_cache.flag_name) - 1);
}