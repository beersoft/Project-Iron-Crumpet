// dash_helpers.h - Rev 3.1.0
// Universal UI grid coordinate converters, card drawers, PSRAM sprite allocators,
// thermal/sector color helpers, and custom 7-segment character renderers.
#pragma once

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// Forward declaration matching the global display instance
class LGFX;
extern LGFX gfx;

// -----------------------------------------------------------------------------
// Theme Color Fallbacks
// -----------------------------------------------------------------------------
#ifndef DASH_CARD_BG
#define DASH_CARD_BG     0x10A2
#endif

#ifndef DASH_CARD_BORDER
#define DASH_CARD_BORDER 0x2945
#endif

#ifndef DASH_CARD_SUBTLE
#define DASH_CARD_SUBTLE 0x18E3
#endif

#ifndef DASH_TEXT_SLATE
#define DASH_TEXT_SLATE  0x8410
#endif

// -----------------------------------------------------------------------------
// Grid Coordinate to Pixel Calculation
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// Grid Coordinate to Pixel Calculation (Screen-Clamped)
// -----------------------------------------------------------------------------
inline void grid_to_pixels(int row, int col, int row_span, int col_span,
                           int32_t &out_x, int32_t &out_y, int32_t &out_w, int32_t &out_h) {
    const int margin_left   = 16;
    const int margin_top    = 38;
    const int cell_width    = 62;
    const int cell_height   = 56;
    const int gap           = 6;

    // Constrain grid positions to 12 columns and 6 rows
    col      = constrain(col, 1, 12);
    row      = constrain(row, 1, 6);
    col_span = constrain(col_span, 1, 13 - col);
    row_span = constrain(row_span, 1, 7 - row);

    out_x = margin_left + ((col - 1) * (cell_width + gap));
    out_y = margin_top  + ((row - 1) * (cell_height + gap));
    out_w = (col_span * cell_width) + ((col_span - 1) * gap);
    out_h = (row_span * cell_height) + ((row_span - 1) * gap);

    // Hard boundary clamps for 800x480 panel (leaves margin on right)
    if (out_x + out_w > 784) {
        out_w = 784 - out_x;
    }
    // Limit max bottom extent to Y=398 to leave the bottom strip for assists & logo
    if (out_y + out_h > 398) {
        out_h = 398 - out_y;
    }
}
// -----------------------------------------------------------------------------
// Thermal & Sector Color Helpers
// -----------------------------------------------------------------------------
inline uint16_t get_sector_color(uint8_t status) {
    switch (status) {
        case 1:  return TFT_YELLOW;  // In-progress sector
        case 2:  return TFT_GREEN;   // Personal best sector
        case 3:  return TFT_MAGENTA; // Overall session best
        default: return TFT_DARKGRAY;
    }
}

inline uint16_t get_tyre_temp_color(float temp_c) {
    if (temp_c <= 0.0f)   return TFT_DARKGRAY;
    if (temp_c < 70.0f)   return TFT_BLUE;      // Cold
    if (temp_c <= 100.0f) return TFT_GREEN;     // Optimum
    if (temp_c <= 115.0f) return TFT_YELLOW;    // Warm
    return TFT_RED;                             // Overheating
}

inline uint16_t get_brake_temp_color(float brake_c) {
    if (brake_c <= 0.0f)   return TFT_DARKGRAY;
    if (brake_c < 250.0f)  return TFT_BLUE;     // Cold
    if (brake_c <= 600.0f) return TFT_GREEN;    // Optimum
    if (brake_c <= 750.0f) return TFT_YELLOW;   // Warm
    return TFT_RED;                             // Overheating
}

// -----------------------------------------------------------------------------
// Dynamic PSRAM Sprite Allocation Helper
// -----------------------------------------------------------------------------
inline void create_psram_sprite(LGFX_Sprite*& spr, int width, int height) {
    if (spr != nullptr) {
        spr->deleteSprite();
        delete spr;
        spr = nullptr;
    }
    spr = new LGFX_Sprite(&gfx);
    spr->setColorDepth(16);
    spr->setPsram(true);
    if (!spr->createSprite(width, height)) {
        Serial.printf("[SPRITE ERR] Failed to allocate %dx%d in PSRAM, falling back to SRAM\n", width, height);
        spr->setPsram(false);
        spr->createSprite(width, height);
    }
}

// -----------------------------------------------------------------------------
// Card & Text Primitives (Device & Sprite Overloads)
// -----------------------------------------------------------------------------
template <typename T>
inline void draw_card_label(T* target, int32_t x, int32_t y, const char* label,
                            uint8_t datum = TL_DATUM, uint16_t color = DASH_TEXT_SLATE) {
    target->setFont(&fonts::FreeSansBold9pt7b);
    target->setTextSize(0.55f);
    target->setTextDatum(datum);
    target->setTextColor(color, DASH_CARD_BG);
    target->drawString(label, x, y);
}

template <typename T>
inline void draw_card_data(T* target, int32_t x, int32_t y, const char* value,
                           float text_size = 1.0f, uint16_t color = TFT_WHITE,
                           uint8_t datum = MC_DATUM, const lgfx::IFont* font = &fonts::Font7) {
    target->setFont(font);
    target->setTextSize(text_size);
    target->setTextDatum(datum);
    target->setTextColor(color, DASH_CARD_BG);
    target->drawString(value, x, y);
}

template <typename T>
inline void draw_card(T* target, int start_row, int start_col, int row_span, int col_span,
                      const char* label, const char* contents, uint16_t box_color, uint16_t text_color,
                      float data_scale = 1.0f, const lgfx::IFont* data_font = &fonts::Font7) {
    int32_t x, y, w, h;
    grid_to_pixels(start_row, start_col, row_span, col_span, x, y, w, h);

    target->fillRect(x, y, w, h, DASH_CARD_BG);
    target->drawRoundRect(x, y, w, h, 6, DASH_CARD_BORDER);

    if (label && strlen(label) > 0) {
        draw_card_label(target, x + 8, y + 6, label, TL_DATUM, DASH_TEXT_SLATE);
    }

    if (contents && strlen(contents) > 0) {
        int content_y = (label && strlen(label) > 0) ? (y + (h / 2) + 6) : (y + (h / 2));
        draw_card_data(target, x + (w / 2), content_y, contents, data_scale, text_color, MC_DATUM, data_font);
    }
}

// -----------------------------------------------------------------------------
// Card Containers, Headers & UI Pills
// -----------------------------------------------------------------------------
inline void draw_grid_group_boundary(LGFX_Device* dst, int x, int y, int w, int h, uint16_t border_col, uint16_t glow_col) {
    dst->drawRoundRect(x, y, w, h, 8, border_col);
    dst->drawRoundRect(x + 1, y + 1, w - 2, h - 2, 7, glow_col);
}

inline void draw_boxed_card_with_verticals(LGFX_Device* dst, int x, int y, int w, int h, uint16_t accent_color) {
    dst->fillRect(x, y, w, h, TFT_BLACK);
    dst->drawRoundRect(x, y, w, h, 6, DASH_CARD_BORDER);
    dst->drawFastHLine(x + 4, y + 1, w - 8, accent_color);
    dst->drawFastHLine(x + 4, y + h - 2, w - 8, accent_color);
}

template <typename T>
inline void draw_full_width_header(T* target, int x, int y, int w, int h, const char* label, uint16_t border_col, uint16_t text_col, uint16_t bg_col, float text_size) {
    target->fillRoundRect(x, y, w, h, 4, bg_col);
    target->drawRoundRect(x, y, w, h, 4, border_col);
    target->setTextDatum(MC_DATUM);
    target->setFont(&fonts::FreeSansBold12pt7b);
    target->setTextSize(text_size);
    target->setTextColor(text_col, bg_col);
    target->drawString(label, x + (w / 2), y + (h / 2));
}

template <typename T>
inline void draw_indicator_pill(T* target, int x, int y, int w, int h, const char* label, uint16_t border_col, uint16_t text_col, uint16_t bg_col, float text_size = 0.55f) {
    target->fillRoundRect(x, y, w, h, 6, bg_col);
    target->drawRoundRect(x, y, w, h, 6, border_col);
    target->setTextDatum(MC_DATUM);
    target->setFont(&fonts::FreeSansBold12pt7b);
    target->setTextSize(text_size);
    target->setTextColor(text_col, bg_col);
    target->drawString(label, x + (w / 2), y + (h / 2));
}

inline void draw_vertical_label_pill(LGFX_Sprite* spr, int x, int y, int w, int h, const char* label, uint16_t border_col, uint16_t text_col, uint16_t bg_col) {
    spr->fillRoundRect(x, y, w, h, 4, bg_col);
    spr->drawRoundRect(x, y, w, h, 4, border_col);
    spr->setFont(&fonts::FreeSansBold12pt7b);
    spr->setTextSize(0.40f);
    spr->setTextColor(text_col, bg_col);
    spr->setTextDatum(MC_DATUM);

    int len = strlen(label);
    if (len == 0) return;
    int step_y = h / (len + 1);
    for (int i = 0; i < len; i++) {
        char ch[2] = { label[i], '\0' };
        spr->drawString(ch, x + (w / 2), y + ((i + 1) * step_y));
    }
}

// -----------------------------------------------------------------------------
// Custom 7-Segment Renderers (Gear 'N' and 'R')
// -----------------------------------------------------------------------------
inline void draw_segment_gear_N(LGFX_Sprite* spr, int cx, int cy, uint16_t on_color = TFT_RED, uint16_t ghost_color = 0x2800) {
    if (!spr) return;

    const int seg_w   = 130;
    const int seg_h   = 220;
    const int t       = 24;
    const int half_h  = seg_h / 2;
    const int x0      = cx - (seg_w / 2);
    const int y0      = cy - (seg_h / 2);
    const int x1      = x0 + seg_w;
    const int y_mid   = y0 + half_h;
    const int y1      = y0 + seg_h;

    // Segment 'a' (Top - GHOST)
    spr->fillTriangle(x0 + t, y0,       x1 - t, y0,       x1 - t*2, y0 + t, ghost_color);
    spr->fillTriangle(x0 + t, y0,       x1 - t*2, y0 + t, x0 + t*2, y0 + t, ghost_color);

    // Segment 'f' (Upper-Left - GHOST)
    spr->fillTriangle(x0, y0 + t,       x0 + t, y0 + t*2, x0 + t, y_mid - t/2, ghost_color);
    spr->fillTriangle(x0, y0 + t,       x0 + t, y_mid - t/2, x0, y_mid, ghost_color);

    // Segment 'b' (Upper-Right - GHOST)
    spr->fillTriangle(x1, y0 + t,       x1 - t, y0 + t*2, x1 - t, y_mid - t/2, ghost_color);
    spr->fillTriangle(x1, y0 + t,       x1 - t, y_mid - t/2, x1, y_mid, ghost_color);

    // Segment 'd' (Bottom - GHOST)
    spr->fillTriangle(x0 + t, y1,       x1 - t, y1,       x1 - t*2, y1 - t, ghost_color);
    spr->fillTriangle(x0 + t, y1,       x1 - t*2, y1 - t, x0 + t*2, y1 - t, ghost_color);

    // Segment 'g' (Middle - ON)
    spr->fillTriangle(x0 + t, y_mid,    x0 + t*2, y_mid - t/2, x1 - t*2, y_mid - t/2, on_color);
    spr->fillTriangle(x0 + t, y_mid,    x1 - t*2, y_mid - t/2, x1 - t, y_mid, on_color);
    spr->fillTriangle(x0 + t, y_mid,    x1 - t, y_mid,         x1 - t*2, y_mid + t/2, on_color);
    spr->fillTriangle(x0 + t, y_mid,    x1 - t*2, y_mid + t/2, x0 + t*2, y_mid + t/2, on_color);

    // Segment 'e' (Lower-Left - ON)
    spr->fillTriangle(x0, y_mid,        x0 + t, y_mid + t/2,   x0 + t, y1 - t*2, on_color);
    spr->fillTriangle(x0, y_mid,        x0 + t, y1 - t*2,     x0, y1 - t, on_color);

    // Segment 'c' (Lower-Right - ON)
    spr->fillTriangle(x1, y_mid,        x1 - t, y_mid + t/2,   x1 - t, y1 - t*2, on_color);
    spr->fillTriangle(x1, y_mid,        x1 - t, y1 - t*2,     x1, y1 - t, on_color);

    // Decimal Point (Ghost)
    spr->fillRect(x1 + 6, y1 - t, t - 4, t, ghost_color);
}

inline void draw_segment_gear_R(LGFX_Sprite* spr, int cx, int cy, uint16_t on_color = TFT_RED, uint16_t ghost_color = 0x2800) {
    if (!spr) return;

    const int seg_w   = 130;
    const int seg_h   = 220;
    const int t       = 24;
    const int half_h  = seg_h / 2;
    const int x0      = cx - (seg_w / 2);
    const int y0      = cy - (seg_h / 2);
    const int x1      = x0 + seg_w;
    const int y_mid   = y0 + half_h;
    const int y1      = y0 + seg_h;

    // Segment 'a' (Top - ON)
    spr->fillTriangle(x0 + t, y0,       x1 - t, y0,       x1 - t*2, y0 + t, on_color);
    spr->fillTriangle(x0 + t, y0,       x1 - t*2, y0 + t, x0 + t*2, y0 + t, on_color);

    // Segment 'f' (Upper-Left - ON)
    spr->fillTriangle(x0, y0 + t,       x0 + t, y0 + t*2, x0 + t, y_mid - t/2, on_color);
    spr->fillTriangle(x0, y0 + t,       x0 + t, y_mid - t/2, x0, y_mid, on_color);

    // Segment 'e' (Lower-Left - ON)
    spr->fillTriangle(x0, y_mid,        x0 + t, y_mid + t/2,   x0 + t, y1 - t*2, on_color);
    spr->fillTriangle(x0, y_mid,        x0 + t, y1 - t*2,     x0, y1 - t, on_color);

    // Segment 'b' (Upper-Right - GHOST)
    spr->fillTriangle(x1, y0 + t,       x1 - t, y0 + t*2, x1 - t, y_mid - t/2, ghost_color);
    spr->fillTriangle(x1, y0 + t,       x1 - t, y_mid - t/2, x1, y_mid, ghost_color);

    // Segment 'g' (Middle - GHOST)
    spr->fillTriangle(x0 + t, y_mid,    x0 + t*2, y_mid - t/2, x1 - t*2, y_mid - t/2, ghost_color);
    spr->fillTriangle(x0 + t, y_mid,    x1 - t*2, y_mid - t/2, x1 - t, y_mid, ghost_color);
    spr->fillTriangle(x0 + t, y_mid,    x1 - t, y_mid,         x1 - t*2, y_mid + t/2, ghost_color);
    spr->fillTriangle(x0 + t, y_mid,    x1 - t*2, y_mid + t/2, x0 + t*2, y_mid + t/2, ghost_color);

    // Segment 'c' (Lower-Right - GHOST)
    spr->fillTriangle(x1, y_mid,        x1 - t, y_mid + t/2,   x1 - t, y1 - t*2, ghost_color);
    spr->fillTriangle(x1, y_mid,        x1 - t, y1 - t*2,     x1, y1 - t, ghost_color);

    // Segment 'd' (Bottom - GHOST)
    spr->fillTriangle(x0 + t, y1,       x1 - t, y1,       x1 - t*2, y1 - t, ghost_color);
    spr->fillTriangle(x0 + t, y1,       x1 - t*2, y1 - t, x0 + t*2, y1 - t, ghost_color);

    // Decimal Point (Ghost)
    spr->fillRect(x1 + 6, y1 - t, t - 4, t, ghost_color);
}