// =============================================================================
// FILE: dash_renderers.cpp
// DESCRIPTION:
//   Implements low-level graphical renderers for Project Iron Crumpet dashboard 
//   elements: 10-segment LED bar gauges, 200° radial arc dials, 2D proximity 
//   radar, static element skeletons, and crisp vector font readouts.
// =============================================================================

#include "dash_renderers.h"
#include "UI.h"
#include "ui_settings.h"
#include "simhub_client_ws.h"
#include <math.h>

// -----------------------------------------------------------------------------
// Function: dim_color565
// -----------------------------------------------------------------------------
uint16_t dim_color565(uint16_t color, float scale) {
    if (scale <= 0.0f) return 0x0000;
    if (scale >= 1.0f) return color;

    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5)  & 0x3F;
    uint8_t b = color & 0x1F;

    r = (uint8_t)((float)r * scale);
    g = (uint8_t)((float)g * scale);
    b = (uint8_t)((float)b * scale);

    return (r << 11) | (g << 5) | b;
}

// -----------------------------------------------------------------------------
// Function: draw_element_skeleton
// Description: Renders card container and header label using FreeSansOblique12pt7b.
// -----------------------------------------------------------------------------
void draw_element_skeleton(const DashElementDef& el) {
    int x, y, w, h;
    grid_to_pixels(el.row, el.col, el.row_span, el.col_span, x, y, w, h);

    if (el.type == TYPE_DIAL || el.type == TYPE_LIGHTBOX || el.type == TYPE_ICON) {
        return;
    }

    // Outer card container
    gfx.fillRoundRect(x, y, w, h, 8, DASH_CARD_BG);
    gfx.drawRoundRect(x, y, w, h, 8, DASH_CARD_BORDER);

    if (el.type == TYPE_BAR) return;

    char label_buf[32] = {0};
    if (el.type == TYPE_SPEED) {
        if (el.label && strlen(el.label) > 0) {
            snprintf(label_buf, sizeof(label_buf), "%s (%s)", el.label, app_config.use_mph ? "MPH" : "KPH");
        } else {
            snprintf(label_buf, sizeof(label_buf), "%s", app_config.use_mph ? "MPH" : "KPH");
        }
    } else if (el.label && strlen(el.label) > 0) {
        strncpy(label_buf, el.label, sizeof(label_buf) - 1);
    }

    if (strlen(label_buf) > 0) {
        // Card labels use FreeSansOblique (slanted)
        gfx.setFont(&fonts::FreeSansOblique12pt7b);
        gfx.setTextSize(1.0f);
        gfx.setTextDatum(ML_DATUM);
        gfx.setTextColor(DASH_TEXT_SLATE, DASH_CARD_BG);

        gfx.setClipRect(x + 4, y + 2, w - 8, 22);
        gfx.drawString(label_buf, x + 10, y + 14);
        gfx.clearClipRect();
    }
}

// -----------------------------------------------------------------------------
// Function: draw_data_value_autofit
// Description: Automatically selects the largest native regular FreeSans font size
//              (not bold, not oblique) that fits the container at 1:1 scale.
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// Function: draw_data_value_autofit
// Description: Automatically scales the flash-loaded TrueType font to fill 
//              the container vertically while constraining horizontal bounds.
// -----------------------------------------------------------------------------
void draw_data_value_autofit(int x, int y, int w, int h, const char* val_str, uint16_t color, bool has_label) {
    if (!val_str || val_str[0] == '\0') return;

    int label_offset = has_label ? 20 : 0;
    int content_x = x + 4;
    int content_y = y + label_offset;
    int content_w = w - 8;
    int content_h = h - label_offset - 4;

    if (content_w <= 0 || content_h <= 0) return;

    int cx = content_x + (content_w / 2);
    int cy = content_y + (content_h / 2);
    int len = strlen(val_str);
    int max_w = content_w - 4;

    // nullptr activates the TTF font loaded via gfx.loadFont() in setup_custom_fonts()
    gfx.setFont(nullptr);
    gfx.setTextColor(color, DASH_CARD_BG);
    gfx.setTextDatum(MC_DATUM);

    // Compute exact pixel point height to fill ~85% of the card height vertically
    int target_pt = (int)((float)content_h * 0.85f);
    if (target_pt < 14) target_pt = 14;
    gfx.setTextSize(target_pt);

    // Horizontal overflow protection: shrink point size proportionally if too wide
    int rendered_w = gfx.textWidth(val_str);
    if (rendered_w > max_w && rendered_w > 0) {
        float ratio = (float)max_w / (float)rendered_w;
        target_pt = (int)((float)target_pt * ratio);
        if (target_pt < 12) target_pt = 12;
        gfx.setTextSize(target_pt);
    }

    gfx.setClipRect(x + 2, y + 2, w - 4, h - 4);
    gfx.drawString(val_str, cx, cy);
    gfx.clearClipRect();
}
// End of draw_data_value_autofit
// -----------------------------------------------------------------------------
// Function: draw_bar_element
// -----------------------------------------------------------------------------
void draw_bar_element(const DashElementDef& el, float val, bool full_draw) {
    int x, y, w, h;
    grid_to_pixels(el.row, el.col, el.row_span, el.col_span, x, y, w, h);

    float min_v = el.source.min_val;
    float max_v = el.source.max_val > min_v ? el.source.max_val : 100.0f;
    float range = max_v - min_v;
    float pct   = (range > 0.0f) ? (val - min_v) / range : 0.0f;
    pct = constrain(pct, 0.0f, 1.0f);

    bool is_rpm = (el.label && strstr(el.label, "RPM") != nullptr);
    bool is_ers = (el.label && strstr(el.label, "ERS") != nullptr);

    if (full_draw) {
        gfx.fillRect(x + 1, y + 1, w - 2, h - 2, DASH_BG_GLOBAL);
        gfx.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 4, DASH_CARD_BORDER);

        if (el.label && strlen(el.label) > 0) {
            gfx.setFont(DASH_FONT_LABEL);
            gfx.setTextSize(1.0f);
            gfx.setTextDatum(TL_DATUM);
            gfx.setTextColor(DASH_TEXT_SLATE, DASH_BG_GLOBAL);
            gfx.drawString(el.label, x + 6, y + 5);
        }
    }

    const int num_segments = 10;
    const int gap          = 2;
    int inner_pad          = 4;

    bool has_header_label = (el.label && strlen(el.label) > 0);
    int track_x = x + inner_pad;
    int track_y = y + (has_header_label ? 20 : inner_pad);
    int track_w = w - (inner_pad * 2);
    int track_h = h - (has_header_label ? 24 : (inner_pad * 2));

    bool is_vertical = (track_h > track_w);
    float total_active_segs = pct * (float)num_segments;

    if (is_vertical) {
        int total_gap_h = (num_segments - 1) * gap;
        int seg_h = (track_h - total_gap_h) / num_segments;
        if (seg_h < 1) seg_h = 1;

        for (int s = 0; s < num_segments; s++) {
            int sy = track_y + track_h - (s * (seg_h + gap)) - seg_h;

            uint16_t lit_col = el.color;
            uint16_t dim_col = dim_color565(el.color, 0.22f);

            if (is_rpm) {
                if (s < 6)      { lit_col = TFT_GREEN;  dim_col = 0x0200; }
                else if (s < 9) { lit_col = TFT_YELLOW; dim_col = 0x4A00; }
                else            { lit_col = TFT_RED;    dim_col = 0x4000; }
            } else if (is_ers) {
                lit_col = 0x9FE0;
                dim_col = 0x22A0;
            }

            if (total_active_segs >= (float)(s + 1)) {
                gfx.fillRoundRect(track_x, sy, track_w, seg_h, 2, lit_col);
            } else if (total_active_segs > (float)s) {
                float partial_pct = total_active_segs - (float)s;
                int lit_h = (int)((float)seg_h * partial_pct);
                int dim_h = seg_h - lit_h;

                if (dim_h > 0) gfx.fillRect(track_x, sy, track_w, dim_h, dim_col);
                if (lit_h > 0) gfx.fillRect(track_x, sy + dim_h, track_w, lit_h, lit_col);
                gfx.drawRoundRect(track_x, sy, track_w, seg_h, 2, DASH_CARD_BORDER);
            } else {
                gfx.fillRoundRect(track_x, sy, track_w, seg_h, 2, dim_col);
            }
        }
    } else {
        int total_gap_w = (num_segments - 1) * gap;
        int seg_w = (track_w - total_gap_w) / num_segments;
        if (seg_w < 1) seg_w = 1;

        for (int s = 0; s < num_segments; s++) {
            int sx = track_x + s * (seg_w + gap);

            uint16_t lit_col = el.color;
            uint16_t dim_col = dim_color565(el.color, 0.22f);

            if (is_rpm) {
                if (s < 6)      { lit_col = TFT_GREEN;  dim_col = 0x0200; }
                else if (s < 9) { lit_col = TFT_YELLOW; dim_col = 0x4A00; }
                else            { lit_col = TFT_RED;    dim_col = 0x4000; }
            } else if (is_ers) {
                lit_col = 0x9FE0;
                dim_col = 0x22A0;
            }

            if (total_active_segs >= (float)(s + 1)) {
                gfx.fillRoundRect(sx, track_y, seg_w, track_h, 2, lit_col);
            } else if (total_active_segs > (float)s) {
                float partial_pct = total_active_segs - (float)s;
                int lit_w = (int)((float)seg_w * partial_pct);
                int dim_w = seg_w - lit_w;

                if (lit_w > 0) gfx.fillRect(sx, track_y, lit_w, track_h, lit_col);
                if (dim_w > 0) gfx.fillRect(sx + lit_w, track_y, dim_w, track_h, dim_col);
                gfx.drawRoundRect(sx, track_y, seg_w, track_h, 2, DASH_CARD_BORDER);
            } else {
                gfx.fillRoundRect(sx, track_y, seg_w, track_h, 2, dim_col);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Function: draw_dial_element
// -----------------------------------------------------------------------------
void draw_dial_element(const DashElementDef& el, float val, bool full_draw) {
    int x, y, w, h;
    grid_to_pixels(el.row, el.col, el.row_span, el.col_span, x, y, w, h);

    int pad = 6;
    int box_x = x + pad;
    int box_y = y + pad;
    int box_w = w - (pad * 2);
    int box_h = h - (pad * 2);

    int r_out = (box_w / 2) - 2; 
    int cx = box_x + (box_w / 2);
    int cy = box_y + r_out; 
    int r_in  = r_out - 16;

    float max_v = 300.0f;
    bool is_speed_dial = (el.label && strstr(el.label, "SPEED") != nullptr);
    bool is_rpm_dial   = (el.label && strstr(el.label, "RPM") != nullptr);

    if (is_speed_dial) {
        if (strlen(live_sim_data.car_class) > 0) {
            if (strcasestr(live_sim_data.car_class, "F1") || strcasestr(live_sim_data.car_class, "Formula") ||
                strcasestr(live_sim_data.car_class, "LMP") || strcasestr(live_sim_data.car_class, "Hypercar") ||
                strcasestr(live_sim_data.car_class, "GTP")) {
                max_v = 380.0f;
            } else if (strcasestr(live_sim_data.car_class, "GT3") || strcasestr(live_sim_data.car_class, "GTE") ||
                       strcasestr(live_sim_data.car_class, "GT4") || strcasestr(live_sim_data.car_class, "Cup") ||
                       strcasestr(live_sim_data.car_class, "GT")) {
                max_v = 300.0f;
            } else {
                max_v = 240.0f;
            }
        }
        if (app_config.use_mph) {
            if (max_v >= 380.0f)      max_v = 240.0f;
            else if (max_v >= 300.0f) max_v = 190.0f;
            else                      max_v = 150.0f;
        }
    } else if (is_rpm_dial) {
        max_v = el.source.max_val > 0.0f ? el.source.max_val : 8500.0f;
    } else {
        max_v = el.source.max_val > 0.0f ? el.source.max_val : 100.0f;
    }

    float min_v = el.source.min_val;
    float range = max_v - min_v;
    float pct   = (range > 0.0f) ? (val - min_v) / range : 0.0f;
    pct = constrain(pct, 0.0f, 1.0f);

    const float start_angle = 145.0f;
    const float sweep_angle = 200.0f;

    int num_segments = 10;
    if (is_rpm_dial) {
        num_segments = (int)roundf(max_v / 1000.0f);
        if (num_segments < 4) num_segments = 8;
    } else if (is_speed_dial) {
        num_segments = (int)roundf(max_v / 10.0f);
        if (num_segments < 10) num_segments = 24;
    }

    float total_target_deg = pct * sweep_angle;
    float seg_angle_span   = sweep_angle / (float)num_segments;
    float gap_deg          = (num_segments > 15) ? 0.8f : 1.5f;

    if (full_draw) {
        gfx.fillRect(x + 1, y + 1, w - 2, h - 2, DASH_CARD_BG);
        gfx.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 6, DASH_CARD_BORDER);

        int tick_step_count = is_rpm_dial ? (num_segments) : (num_segments / 3);
        if (tick_step_count < 1) tick_step_count = 1;

        for (int i = 0; i <= tick_step_count; i++) {
            float ratio = (float)i / (float)tick_step_count;
            float tick_ang = (start_angle + (ratio * sweep_angle)) * DEG_TO_RAD;
            int tx = cx + (int)(cosf(tick_ang) * (float)(r_in - 12));
            int ty = cy + (int)(sinf(tick_ang) * (float)(r_in - 12));
            
            char num_buf[8];
            int tick_val = (int)(min_v + (ratio * range));
            
            if (is_rpm_dial) {
                snprintf(num_buf, sizeof(num_buf), "%d", tick_val / 1000);
            } else {
                snprintf(num_buf, sizeof(num_buf), "%d", tick_val);
            }
            
            gfx.setFont(DASH_FONT_LABEL);
            gfx.setTextSize(1.0f);
            gfx.setTextDatum(MC_DATUM);
            gfx.setTextColor(DASH_TEXT_SLATE, DASH_CARD_BG);
            gfx.drawString(num_buf, tx, ty);
        }

        if (is_rpm_dial && strlen(live_sim_data.car_class) > 0) {
            gfx.setFont(DASH_FONT_LABEL);
            gfx.setTextSize(1.0f);
            gfx.setTextDatum(MC_DATUM);
            gfx.setTextColor(DASH_TEXT_SLATE, DASH_CARD_BG);
            gfx.drawString(live_sim_data.car_class, cx, cy - 20);
        }
    }

    for (int s = 0; s < num_segments; s++) {
        float seg_start = start_angle + (s * seg_angle_span) + (gap_deg / 2.0f);
        float seg_end   = seg_start + seg_angle_span - gap_deg;

        float seg_absolute_end   = (float)(s + 1) * seg_angle_span;
        float seg_absolute_start = (float)s * seg_angle_span;

        uint16_t lit_col = el.color;
        uint16_t dim_col = dim_color565(el.color, 0.22f);

        if (is_rpm_dial) {
            if (s == num_segments - 1) {
                lit_col = TFT_RED;    dim_col = 0x4000;
            } else {
                float seg_ratio = (float)s / (float)num_segments;
                if (seg_ratio < 0.70f) {
                    lit_col = TFT_GREEN;  dim_col = 0x0200;
                } else {
                    lit_col = TFT_YELLOW; dim_col = 0x4A00;
                }
            }
        }

        if (total_target_deg >= seg_absolute_end) {
            gfx.fillArc(cx, cy, r_out, r_in, seg_start, seg_end, lit_col);
        } else if (total_target_deg > seg_absolute_start) {
            float partial_pct = (total_target_deg - seg_absolute_start) / seg_angle_span;
            float partial_end = seg_start + (seg_end - seg_start) * partial_pct;
            
            gfx.fillArc(cx, cy, r_out, r_in, seg_start, partial_end, lit_col);
            gfx.fillArc(cx, cy, r_out, r_in, partial_end, seg_end, dim_col);
        } else {
            gfx.fillArc(cx, cy, r_out, r_in, seg_start, seg_end, dim_col);
        }
    }
}

// -----------------------------------------------------------------------------
// Function: draw_radar_element
// -----------------------------------------------------------------------------
void draw_radar_element(const DashElementDef& el, bool full_draw) {
    int x, y, w, h;
    grid_to_pixels(el.row, el.col, el.row_span, el.col_span, x, y, w, h);

    int pad = 4;
    int bx = x + pad;
    int by = y + pad;
    int bw = w - (pad * 2);
    int bh = h - (pad * 2);

    int cx = bx + (bw / 2);
    int cy = by + (bh / 2);

    if (full_draw) {
        gfx.fillRect(x + 1, y + 1, w - 2, h - 2, DASH_CARD_BG);
        gfx.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 6, DASH_CARD_BORDER);

        if (el.label && strlen(el.label) > 0) {
            gfx.setFont(DASH_FONT_LABEL);
            gfx.setTextSize(1.0f);
            gfx.setTextDatum(TL_DATUM);
            gfx.setTextColor(DASH_TEXT_SLATE, DASH_CARD_BG);
            gfx.drawString(el.label, bx + 6, by + 4);
        }
    } else {
        gfx.fillRect(bx + 2, by + 16, bw - 4, bh - 18, DASH_CARD_BG);
    }

    int max_radius = min(bw, bh) / 2 - 8;
    int r_close = (int)((float)max_radius * 0.30f);
    int r_mid   = (int)((float)max_radius * 0.65f);
    int r_far   = max_radius;

    uint16_t ring_col = 0x18E3;
    gfx.drawCircle(cx, cy, r_close, ring_col);
    gfx.drawCircle(cx, cy, r_mid, ring_col);
    gfx.drawCircle(cx, cy, r_far, ring_col);
    gfx.drawFastHLine(cx - max_radius, cy, max_radius * 2, ring_col);
    gfx.drawFastVLine(cx, cy - max_radius, max_radius * 2, ring_col);

    int strip_w = 4;
    if (live_sim_data.spotter_left > 0) {
        gfx.fillRoundRect(bx + 2, by + 18, strip_w, bh - 24, 2, TFT_RED);
    }
    if (live_sim_data.spotter_right > 0) {
        gfx.fillRoundRect(bx + bw - strip_w - 2, by + 18, strip_w, bh - 24, 2, TFT_RED);
    }

    int player_w = 8;
    int player_h = 14;
    gfx.fillRoundRect(cx - (player_w / 2), cy - (player_h / 2), player_w, player_h, 2, TFT_CYAN);

    float px_per_meter = (float)max_radius / 35.0f;

    float ahead_dist = fabsf(live_sim_data.opponent_ahead_dist);
    if (ahead_dist > 0.5f && ahead_dist < 35.0f) {
        int opp_y = cy - (int)(ahead_dist * px_per_meter);
        uint16_t opp_col = (ahead_dist < 8.0f) ? TFT_RED : TFT_YELLOW;
        gfx.fillRoundRect(cx - 4, opp_y - 6, 8, 12, 2, opp_col);
    }

    float behind_dist = fabsf(live_sim_data.opponent_behind_dist);
    if (behind_dist > 0.5f && behind_dist < 35.0f) {
        int opp_y = cy + (int)(behind_dist * px_per_meter);
        int opp_x = cx + (int)(live_sim_data.opponent_behind_offset * (px_per_meter * 2.0f));
        opp_x = constrain(opp_x, bx + 8, bx + bw - 8);

        uint16_t opp_col = (behind_dist < 8.0f) ? TFT_RED : TFT_YELLOW;
        gfx.fillRoundRect(opp_x - 4, opp_y - 6, 8, 12, 2, opp_col);
    }
}