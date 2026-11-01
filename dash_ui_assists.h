// dash_ui_assists.h
#pragma once

#include "dash_ui_common.h"


inline void update_assists_tray_ui(const SimTelemetry& snap) {
    if (!sprAssists) return;

    sprAssists->fillSprite(DASH_BG_GLOBAL);

    const int size = 72;
    const int gap  = 6;

    auto get_pill_colors = [&](bool exists, bool active, uint16_t accent_color, 
                               uint16_t &out_bg, uint16_t &out_fg, uint16_t &out_border) {
        if (!exists) {
            out_bg     = DASH_BG_GLOBAL;
            out_fg     = DASH_TEXT_SLATE;
            out_border = DASH_TEXT_SLATE;
        } else if (!active) {
            out_bg     = DASH_BG_GLOBAL;
            out_fg     = accent_color;
            out_border = accent_color;
        } else {
            out_bg     = accent_color;
            out_fg     = TFT_BLACK;
            out_border = accent_color;
        }
    };

// Helper lambda to draw a centered, padded 1-bit XBM icon
    auto draw_iso_pill = [&](int x, int y, const uint8_t* icon_data, uint16_t bg, uint16_t fg, uint16_t border) {
        sprAssists->fillRoundRect(x, y, size, size, 8, bg);
        sprAssists->drawRoundRect(x, y, size, size, 8, border);
        
        // 72px pill box minus 56px icon = 16px total space -> 8px offset for clean centering & padding
        sprAssists->drawXBitmap(x + 8, y + 8, icon_data, 56, 56, fg);
    };
    // ----------------------------------------------------
    // 1. ABS Pill
    // ----------------------------------------------------
    {
        int x = 2 + (0 * (size + gap));
        uint16_t bg, fg, border;
        get_pill_colors(snap.abs_available, snap.abs_active, TFT_YELLOW, bg, fg, border);
        draw_iso_pill(x, 2, ICON_ISO_0239, bg, fg, border);
    }

    // ----------------------------------------------------
    // 2. TC Pill (Using ISO 2630)
    // ----------------------------------------------------
    {
        int x = 2 + (1 * (size + gap));
        uint16_t bg, fg, border;
        get_pill_colors(snap.tc_available, snap.tc_active, TFT_BLUE, bg, fg, border);
        draw_iso_pill(x, 2, ICON_ISO_2630, bg, fg, border);
    }

    // ----------------------------------------------------
    // 3. PIT Pill (Using ISO 2574)
    // ----------------------------------------------------
    {
        int x = 2 + (2 * (size + gap));
        bool available = snap.pit_limiter_available || snap.pit_limiter_on;
        bool active    = snap.pit_limiter_on;
        bool flash_state = (millis() / 200) % 2 == 0;

        uint16_t bg, fg, border;
        if (active) {
            if (flash_state) {
                bg     = TFT_BLUE;
                fg     = TFT_WHITE;
                border = TFT_CYAN;
            } else {
                bg     = TFT_ORANGE;
                fg     = TFT_BLACK;
                border = TFT_YELLOW;
            }
        } else {
            get_pill_colors(available, false, TFT_ORANGE, bg, fg, border);
        }

        draw_iso_pill(x, 2, ICON_ISO_2574, bg, fg, border);
    }

    // ----------------------------------------------------
    // 4. LIGHTS Pill
    // ----------------------------------------------------
    {
        int x = 2 + (3 * (size + gap));
        uint16_t accent = snap.highbeam_flash_on ? TFT_CYAN : TFT_GREEN;
        bool active = snap.highbeam_flash_on || snap.headlights_on;

        uint16_t bg, fg, border;
        get_pill_colors(snap.lights_available, active, accent, bg, fg, border);
        draw_iso_pill(x, 2, ICON_ISO_0082, bg, fg, border);
    }

    // ----------------------------------------------------
    // 5. WIPER Pill
    // ----------------------------------------------------
    {
        int x = 2 + (4 * (size + gap));
        uint16_t bg, fg, border;
        get_pill_colors(snap.wipers_available, snap.wipers_on, TFT_MAGENTA, bg, fg, border);
        draw_iso_pill(x, 2, ICON_ISO_0086, bg, fg, border);
    }

    // ----------------------------------------------------
    // 6. ENGINE / IGNITION Pill
    // ----------------------------------------------------
    {
        int x = 2 + (5 * (size + gap));
        uint16_t bg, fg, border;
        get_pill_colors(snap.ignition_available, snap.engine_running, TFT_RED, bg, fg, border);
        draw_iso_pill(x, 2, ICON_ISO_0640, bg, fg, border);
    }

    // ----------------------------------------------------
    // 7. BEERSOFT LOGO BADGE (Integrated right-hand block)
    // ----------------------------------------------------
    {
        int logo_x = 2 + (6 * (size + gap)) + 4;
        draw_assist_bar_logo(sprAssists, logo_x, 2, 286, 72);
    }

    sprAssists->pushSprite(16, 402);
}