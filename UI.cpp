// =============================================================================
// FILE: UI.cpp
// DESCRIPTION:
//   Implements the core user interface state machine, touch event handling,
//   quick-action control menus, diagnostic overlays, declarative alert triggers,
//   and the high-frequency declarative rendering loop for Project Iron Crumpet.
// =============================================================================

#include "UI.h"
#include "dash_renderers.h"
#include <WiFi.h>

// Active dashboard page and alert instances
DashboardPage active_page = PAGE_RACE;
SettingAlert active_alert = { false, false, "", "", 0, 0 };

// Backlight and Display Brightness Definitions
const uint8_t brightness_levels[4] = { 255, 150, 60, 25 };
uint8_t current_brightness_idx     = 0;

// Idle Sleep Timeout Configuration (0 = OFF, 1m, 3m, 5m)
static const uint32_t timeout_values[4] = { 0, 60000, 180000, 300000 };
const char* timeout_labels[4]           = { "OFF", "1 MIN", "3 MIN", "5 MIN" };
uint8_t current_timeout_idx             = 0;

// Internal UI State Tracking
static bool     screen_sleeping    = false;
static uint32_t last_touch_wake_ms = 0;

bool    page_menu_open   = false;
uint8_t active_menu_tab  = 1;
bool    modal_popup_open = false;

// =============================================================================
// HARDWARE & UTILITY HELPERS
// =============================================================================

// -----------------------------------------------------------------------------
// Function: check_screen_sleep
// Description: Monitors telemetry stream activity and user touch input against
//              the configured idle timeout. Powers down the backlight when idle,
//              and restores configured brightness when activity resumes.
// -----------------------------------------------------------------------------
void check_screen_sleep() {
    uint32_t timeout = timeout_values[current_timeout_idx];
    if (timeout == 0) return;

    uint32_t now = millis();
    bool should_sleep = !live_sim_data.connected && 
                        (now - live_sim_data.last_packet_ms > timeout) &&
                        (now - last_touch_wake_ms > timeout);

    if (should_sleep && !screen_sleeping) {
        screen_sleeping = true;
        gfx.setBrightness(0);
    } else if (!should_sleep && screen_sleeping) {
        screen_sleeping = false;
        gfx.setBrightness(brightness_levels[current_brightness_idx]);
    }
}
// End of check_screen_sleep

// -----------------------------------------------------------------------------
// Function: run_demo_sweep
// Description: Animates gauges from 0% to 100% to 0% across Speed and RPM
//              channels to verify visual scale mapping and interpolation.
// -----------------------------------------------------------------------------
void run_demo_sweep() {
    float original_speed = live_sim_data.speed_kmh;
    int   original_rpm   = live_sim_data.rpm;

    for (int i = 0; i <= 100; i += 2) {
        live_sim_data.speed_kmh = (float)i * 3.2f;
        live_sim_data.rpm       = (int)((float)i * 85.0f);
        update_declarative_ui();
        delay(10);
    }
    for (int i = 100; i >= 0; i -= 2) {
        live_sim_data.speed_kmh = (float)i * 3.2f;
        live_sim_data.rpm       = (int)((float)i * 85.0f);
        update_declarative_ui();
        delay(10);
    }

    live_sim_data.speed_kmh = original_speed;
    live_sim_data.rpm       = original_rpm;
    render_declarative_page_static();
}
// End of run_demo_sweep

// -----------------------------------------------------------------------------
// Function: toggle_speed_unit
// Description: Toggles the global unit preference between KPH and MPH, commits
//              the value to NVS flash, and invalidates all speed-dependent elements.
// -----------------------------------------------------------------------------
void toggle_speed_unit() {
    app_config.use_mph = !app_config.use_mph;
    save_config_to_nvs();

    for (size_t i = 0; i < NUM_DASH_ELEMENTS; i++) {
        if (dash_elements[i].type == TYPE_SPEED || 
            dash_elements[i].source.f_ptr == &live_sim_data.speed_kmh) {
            dash_elements[i].runtime.needs_full_draw = true;
            dash_elements[i].runtime.text_cache[0]   = '\0';
        }
    }
}
// End of toggle_speed_unit

// =============================================================================
// MENU & MODAL OVERLAY RENDERERS
// =============================================================================

// -----------------------------------------------------------------------------
// Function: render_page_menu
// Description: Renders the centered 2-column quick-action control menu modal.
// -----------------------------------------------------------------------------
void render_page_menu(int highlight_idx) {
    const int menu_w = 540;
    const int menu_h = 360;
    const int menu_x = (800 - menu_w) / 2;
    const int menu_y = (480 - menu_h) / 2;

    gfx.fillRoundRect(menu_x, menu_y, menu_w, menu_h, 12, DASH_CARD_BG);
    gfx.drawRoundRect(menu_x, menu_y, menu_w, menu_h, 12, DASH_CARD_BORDER);
    gfx.drawRoundRect(menu_x + 1, menu_y + 1, menu_w - 2, menu_h - 2, 12, TFT_WHITE);

    gfx.setFont(DASH_FONT_LABEL);
    gfx.setTextSize(0.85f);
    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(TFT_WHITE, DASH_CARD_BG);

    MenuItemDef* items = (active_menu_tab == 1) ? menu_page1_items : menu_page2_items;
    size_t count       = (active_menu_tab == 1) ? NUM_MENU_PAGE1_ITEMS : NUM_MENU_PAGE2_ITEMS;

    char title_buf[48];
    snprintf(title_buf, sizeof(title_buf), "CONTROL MENU (PAGE %d/2)", active_menu_tab);
    gfx.drawString(title_buf, menu_x + (menu_w / 2), menu_y + 24);

    const int btn_w   = 240;
    const int btn_h   = 56;
    const int gap_x   = 20;
    const int gap_y   = 12;
    const int start_x = menu_x + 20;
    const int start_y = menu_y + 54;

    for (size_t i = 0; i < count; i++) {
        int col = i % 2;
        int row = i / 2;

        int bx = start_x + (col * (btn_w + gap_x));
        int by = start_y + (row * (btn_h + gap_y));

        items[i].x = bx;
        items[i].y = by;
        items[i].w = btn_w;
        items[i].h = btn_h;

        bool is_pressed     = (highlight_idx == (int)i);
        uint16_t bg_col     = is_pressed ? items[i].color : DASH_BG_GLOBAL;
        uint16_t border_col = is_pressed ? TFT_WHITE : DASH_CARD_BORDER;
        uint16_t text_col   = is_pressed ? TFT_BLACK : items[i].color;

        gfx.fillRoundRect(bx, by, btn_w, btn_h, 8, bg_col);
        gfx.drawRoundRect(bx, by, btn_w, btn_h, 8, border_col);

        char label_buf[32];
        if (items[i].action_type == ACTION_TOGGLE_SPEED_UNIT) {
            snprintf(label_buf, sizeof(label_buf), "UNIT: %s", app_config.use_mph ? "MPH" : "KPH");
        } else if (items[i].action_type == ACTION_CYCLE_BRIGHTNESS) {
            snprintf(label_buf, sizeof(label_buf), "BRIGHTNESS: %d%%", (4 - current_brightness_idx) * 25);
        } else if (items[i].action_type == ACTION_CYCLE_IDLE_TIMEOUT) {
            snprintf(label_buf, sizeof(label_buf), "SLEEP: %s", timeout_labels[current_timeout_idx]);
        } else if (items[i].action_type == ACTION_CYCLE_GAME_OVERRIDE) {
            snprintf(label_buf, sizeof(label_buf), "SIM: %s", game_override_names[app_config.game_override_idx]);
        } else if (items[i].action_type == ACTION_CYCLE_CLASS_OVERRIDE) {
            snprintf(label_buf, sizeof(label_buf), "CLASS: %s", class_override_names[app_config.class_override_idx]);
        } else {
            strncpy(label_buf, items[i].label, sizeof(label_buf) - 1);
            label_buf[sizeof(label_buf) - 1] = '\0';
        }

        gfx.setFont(DASH_FONT_LABEL);
        gfx.setTextSize(0.68f);
        gfx.setTextDatum(MC_DATUM);
        gfx.setTextColor(text_col, bg_col);
        gfx.drawString(label_buf, bx + (btn_w / 2), by + (btn_h / 2));
    }
}
// End of render_page_menu

// -----------------------------------------------------------------------------
// Function: render_config_info_popup
// Description: Draws diagnostic overlay with connection, game, and car telemetry.
// -----------------------------------------------------------------------------
void render_config_info_popup() {
    const int popup_w = 640;
    const int popup_h = 400;
    const int popup_x = (800 - popup_w) / 2;
    const int popup_y = (480 - popup_h) / 2;

    gfx.fillRoundRect(popup_x, popup_y, popup_w, popup_h, 12, DASH_CARD_BG);
    gfx.drawRoundRect(popup_x, popup_y, popup_w, popup_h, 12, TFT_GREEN);

    gfx.setFont(DASH_FONT_LABEL);
    gfx.setTextSize(0.85f);
    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(TFT_WHITE, DASH_CARD_BG);
    gfx.drawString("NETWORK & SERVER DIAGNOSTICS", popup_x + (popup_w / 2), popup_y + 26);

    gfx.setTextDatum(ML_DATUM);
    gfx.setTextSize(0.62f);
    int ty = popup_y + 70;

    char buf[128];
    snprintf(buf, sizeof(buf), "WiFi SSID    : %s", app_config.wifi_ssid);
    gfx.drawString(buf, popup_x + 30, ty); ty += 30;

    snprintf(buf, sizeof(buf), "ESP32 IP     : %s", WiFi.localIP().toString().c_str());
    gfx.drawString(buf, popup_x + 30, ty); ty += 30;

    snprintf(buf, sizeof(buf), "SimHub Server: %s:%d", app_config.simhub_ip, app_config.simhub_port);
    gfx.drawString(buf, popup_x + 30, ty); ty += 30;

    const char* active_game = (strlen(live_sim_data.game_name) > 0) ? live_sim_data.game_name : "NO SIM RUNNING";
    snprintf(buf, sizeof(buf), "Active Sim   : %s", active_game);
    gfx.setTextColor(TFT_CYAN, DASH_CARD_BG);
    gfx.drawString(buf, popup_x + 30, ty); ty += 30;

    char car_summary[64];
    snprintf(car_summary, sizeof(car_summary), "Car / Class  : %s [%s]", 
             live_sim_data.car_name, live_sim_data.car_class);
    gfx.setTextColor(TFT_YELLOW, DASH_CARD_BG);
    gfx.drawString(car_summary, popup_x + 30, ty); ty += 30;

    snprintf(buf, sizeof(buf), "Status       : %s", live_sim_data.connected ? "CONNECTED (60 Hz)" : "DISCONNECTED / SEARCHING");
    gfx.setTextColor(live_sim_data.connected ? TFT_GREEN : TFT_RED, DASH_CARD_BG);
    gfx.drawString(buf, popup_x + 30, ty); ty += 42;

    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(DASH_TEXT_SLATE, DASH_CARD_BG);
    gfx.drawString("Tap anywhere to close", popup_x + (popup_w / 2), ty);
}
// End of render_config_info_popup

// -----------------------------------------------------------------------------
// Function: render_terminal_popup
// Description: Live telemetry monitor displaying current stream variables.
// -----------------------------------------------------------------------------
void render_terminal_popup() {
    const int popup_w = 700;
    const int popup_h = 380;
    const int popup_x = (800 - popup_w) / 2;
    const int popup_y = (480 - popup_h) / 2;

    gfx.fillRoundRect(popup_x, popup_y, popup_w, popup_h, 8, TFT_BLACK);
    gfx.drawRoundRect(popup_x, popup_y, popup_w, popup_h, 8, TFT_ORANGE);

    gfx.setFont(DASH_FONT_LABEL);
    gfx.setTextSize(0.70f);
    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(TFT_ORANGE, TFT_BLACK);
    gfx.drawString("SIMHUB INCOMING PACKET MONITOR", popup_x + (popup_w / 2), popup_y + 24);

    gfx.setTextDatum(ML_DATUM);
    gfx.setFont(&fonts::Font0);
    gfx.setTextSize(1.5f);
    gfx.setTextColor(TFT_WHITE, TFT_BLACK);

    char buf[128];
    int ty = popup_y + 60;
    snprintf(buf, sizeof(buf), "Spd: %.1f km/h | RPM: %d | Gear: %s", live_sim_data.speed_kmh, live_sim_data.rpm, live_sim_data.gear);
    gfx.drawString(buf, popup_x + 25, ty); ty += 24;

    snprintf(buf, sizeof(buf), "Lap: %s | Last: %s | Best: %s", live_sim_data.current_lap_time, live_sim_data.last_lap_time, live_sim_data.best_lap_time);
    gfx.drawString(buf, popup_x + 25, ty); ty += 24;

    snprintf(buf, sizeof(buf), "Delta: %+.3f | Bias: %.1f%% | Map: %d", live_sim_data.delta_to_best, live_sim_data.brake_bias, live_sim_data.engine_map);
    gfx.drawString(buf, popup_x + 25, ty); ty += 24;

    snprintf(buf, sizeof(buf), "FL Temp: %.1f C | FR Temp: %.1f C | RL Temp: %.1f C | RR Temp: %.1f C",
             live_sim_data.fl.temp_c, live_sim_data.fr.temp_c, live_sim_data.rl.temp_c, live_sim_data.rr.temp_c);
    gfx.drawString(buf, popup_x + 25, ty); ty += 40;

    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(DASH_TEXT_SLATE, TFT_BLACK);
    gfx.drawString("Tap anywhere to close", popup_x + (popup_w / 2), popup_y + popup_h - 20);
}
// End of render_terminal_popup

// =============================================================================
// MENU ACTION DISPATCH HANDLERS
// =============================================================================

// -----------------------------------------------------------------------------
// Function: menu_action_switch_page
// Description: Changes dashboard active page and commits default to NVS.
// -----------------------------------------------------------------------------
static void menu_action_switch_page(DashboardPage target_page) {
    active_page = target_page;
    app_config.default_page = (uint8_t)active_page;
    save_config_to_nvs();
    page_menu_open = false;

    for (size_t e = 0; e < NUM_DASH_ELEMENTS; e++) {
        if (dash_elements[e].page == active_page) {
            dash_elements[e].runtime.needs_full_draw   = true;
            dash_elements[e].runtime.text_cache[0]     = '\0';
            dash_elements[e].runtime.last_raw_val      = -9999.0f;
            dash_elements[e].runtime.eased_val         = 0.0f;
            dash_elements[e].runtime.last_needle_angle = -1.0f;
        }
    }
    render_declarative_page_static();
}
// End of menu_action_switch_page

static void menu_action_set_tab(uint8_t tab) {
    active_menu_tab = tab;
    render_page_menu();
}
// End of menu_action_set_tab

static void menu_action_toggle_speed_unit() {
    page_menu_open = false;
    toggle_speed_unit();
    render_declarative_page_static();
}
// End of menu_action_toggle_speed_unit

static void menu_action_cycle_brightness() {
    current_brightness_idx = (current_brightness_idx + 1) % 4;
    app_config.brightness_idx = current_brightness_idx;
    save_config_to_nvs();
    gfx.setBrightness(brightness_levels[current_brightness_idx]);
    render_page_menu();
}
// End of menu_action_cycle_brightness

static void menu_action_cycle_idle_timeout() {
    current_timeout_idx = (current_timeout_idx + 1) % 4;
    app_config.idle_timeout_idx = current_timeout_idx;
    save_config_to_nvs();
    render_page_menu();
}
// End of menu_action_cycle_idle_timeout

static void menu_action_cycle_game_override() {
    app_config.game_override_idx = (app_config.game_override_idx + 1) % NUM_GAME_OVERRIDES;
    save_config_to_nvs();

    if (app_config.game_override_idx > 0) {
        strncpy(live_sim_data.game_name, game_override_names[app_config.game_override_idx], sizeof(live_sim_data.game_name) - 1);
    }
    render_page_menu();
}
// End of menu_action_cycle_game_override

static void menu_action_cycle_class_override() {
    app_config.class_override_idx = (app_config.class_override_idx + 1) % NUM_CLASS_OVERRIDES;
    save_config_to_nvs();

    if (app_config.class_override_idx > 0) {
        strncpy(live_sim_data.car_class, class_override_names[app_config.class_override_idx], sizeof(live_sim_data.car_class) - 1);
        for (size_t i = 0; i < NUM_DASH_ELEMENTS; i++) {
            if (dash_elements[i].type == TYPE_DIAL || dash_elements[i].type == TYPE_BAR) {
                dash_elements[i].runtime.needs_full_draw = true;
            }
        }
    }
    render_page_menu();
}
// End of menu_action_cycle_class_override

static void menu_action_demo_sweep() {
    page_menu_open = false;
    render_declarative_page_static();
    run_demo_sweep();
}
// End of menu_action_demo_sweep

static void menu_action_show_config_info() {
    modal_popup_open = true;
    render_config_info_popup();
}
// End of menu_action_show_config_info

static void menu_action_show_terminal() {
    modal_popup_open = true;
    render_terminal_popup();
}
// End of menu_action_show_terminal

static void menu_action_close() {
    page_menu_open = false;
    render_declarative_page_static();
}
// End of menu_action_close

// -----------------------------------------------------------------------------
// Function: execute_menu_item_action
// Description: Routes menu button actions to dedicated handler routines.
// -----------------------------------------------------------------------------
static void execute_menu_item_action(const MenuItemDef& item) {
    switch (item.action_type) {
        case ACTION_SWITCH_PAGE:          menu_action_switch_page(item.target_page);  break;
        case ACTION_MENU_PAGE_2:          menu_action_set_tab(2);                     break;
        case ACTION_MENU_PAGE_1:          menu_action_set_tab(1);                     break;
        case ACTION_TOGGLE_SPEED_UNIT:    menu_action_toggle_speed_unit();            break;
        case ACTION_CYCLE_BRIGHTNESS:     menu_action_cycle_brightness();             break;
        case ACTION_CYCLE_IDLE_TIMEOUT:   menu_action_cycle_idle_timeout();           break;
        case ACTION_CYCLE_GAME_OVERRIDE:  menu_action_cycle_game_override();          break;
        case ACTION_CYCLE_CLASS_OVERRIDE: menu_action_cycle_class_override();         break;
        case ACTION_DEMO_SWEEP:           menu_action_demo_sweep();                   break;
        case ACTION_SHOW_CONFIG_INFO:     menu_action_show_config_info();             break;
        case ACTION_SHOW_TERMINAL:        menu_action_show_terminal();                break;
        case ACTION_CLOSE_MENU:
        default:                          menu_action_close();                        break;
    }
}
// End of execute_menu_item_action

// -----------------------------------------------------------------------------
// Function: handle_ui_touch
// Description: Touch interaction state machine handling wake, popups, and menu hits.
// -----------------------------------------------------------------------------
void handle_ui_touch(uint16_t tx, uint16_t ty, bool is_touched) {
    static uint32_t last_touch_action      = 0;
    static bool     touch_was_down         = false;
    static bool     awaiting_touch_release = false;
    static uint16_t latched_tx             = 0;
    static uint16_t latched_ty             = 0;
    uint32_t now = millis();

    if (is_touched) {
        last_touch_wake_ms = now;

        if (screen_sleeping) {
            screen_sleeping = false;
            gfx.setBrightness(brightness_levels[current_brightness_idx]);
            return;
        }

        latched_tx = tx;
        latched_ty = ty;

        if (!touch_was_down) {
            touch_was_down = true;

            if (modal_popup_open) {
                modal_popup_open = false;
                render_page_menu();
                return;
            }

            if (!page_menu_open && (now - last_touch_action > 300)) {
                last_touch_action      = now;
                page_menu_open         = true;
                active_menu_tab        = 1;
                awaiting_touch_release = true;
                render_page_menu();
                return;
            }
        }
    } else {
        if (touch_was_down) {
            touch_was_down = false;

            if (awaiting_touch_release) {
                awaiting_touch_release = false;
                last_touch_action      = now;
                return;
            }

            if (page_menu_open && (now - last_touch_action > 150)) {
                last_touch_action = now;
                bool item_clicked = false;

                MenuItemDef* items = (active_menu_tab == 1) ? menu_page1_items : menu_page2_items;
                size_t count       = (active_menu_tab == 1) ? NUM_MENU_PAGE1_ITEMS : NUM_MENU_PAGE2_ITEMS;

                for (size_t i = 0; i < count; i++) {
                    MenuItemDef& item = items[i];

                    if (latched_tx >= item.x && latched_tx <= (item.x + item.w) &&
                        latched_ty >= item.y && latched_ty <= (item.y + item.h)) {
                        
                        render_page_menu(i);
                        delay(120);
                        item_clicked = true;
                        execute_menu_item_action(item);
                        break;
                    }
                }

                if (!item_clicked && !modal_popup_open) {
                    menu_action_close();
                }
            }
        }
    }
}
// End of handle_ui_touch

// =============================================================================
// ALERT TRIGGER EVALUATOR HELPERS
// =============================================================================

static bool eval_alert_delta_float(const AlertTriggerDef& trig, AlertRuntimeState& rt, char* val_buf, size_t buf_size) {
    if (!trig.f_ptr) return false;
    float cur = *trig.f_ptr;
    if (!rt.initialised) {
        rt.last_float  = cur;
        rt.initialised = true;
        return false;
    }
    if (fabsf(cur - rt.last_float) >= trig.delta_or_threshold) {
        snprintf(val_buf, buf_size, trig.format ? trig.format : "%.1f", cur);
        rt.last_float = cur;
        return true;
    }
    return false;
}
// End of eval_alert_delta_float

static bool eval_alert_change_int(const AlertTriggerDef& trig, AlertRuntimeState& rt, char* val_buf, size_t buf_size) {
    if (!trig.i_ptr) return false;
    int cur = *trig.i_ptr;
    if (!rt.initialised) {
        rt.last_int    = cur;
        rt.initialised = true;
        return false;
    }
    if (cur != rt.last_int) {
        snprintf(val_buf, buf_size, trig.format ? trig.format : "%d", cur);
        rt.last_int = cur;
        return true;
    }
    return false;
}
// End of eval_alert_change_int

static bool eval_alert_bool_on(const AlertTriggerDef& trig, AlertRuntimeState& rt, char* val_buf, size_t buf_size) {
    if (!trig.b_ptr) return false;
    bool cur = *trig.b_ptr;
    if (!rt.initialised) {
        rt.last_bool   = cur;
        rt.initialised = true;
        return false;
    }
    bool triggered = false;
    if (cur && !rt.last_bool) {
        snprintf(val_buf, buf_size, "ACTIVE");
        triggered = true;
    }
    rt.last_bool = cur;
    return triggered;
}
// End of eval_alert_bool_on

static bool eval_alert_string(const DashAlertDef& alert, const AlertTriggerDef& trig, AlertRuntimeState& rt, char* val_buf, size_t buf_size) {
    if (!trig.s_ptr) return false;
    if (!rt.initialised) {
        strncpy(rt.last_str, trig.s_ptr, sizeof(rt.last_str) - 1);
        rt.initialised = true;
        return false;
    }
    bool triggered = false;
    if (strcmp(trig.s_ptr, rt.last_str) != 0) {
        if (strlen(trig.s_ptr) > 0 &&
            strcmp(trig.s_ptr, "None") != 0 &&
            strcmp(trig.s_ptr, "--:--:--") != 0 &&
            strcmp(trig.s_ptr, "--:--.---") != 0 &&
            strcmp(trig.s_ptr, "--:--:--.-") != 0 &&
            strcmp(trig.s_ptr, "00:00:00") != 0) {

            triggered = true;
            snprintf(val_buf, buf_size, "%s", trig.s_ptr);

            if (strcmp(alert.title, "LAST LAP") == 0) {
                for (int h = 4; h > 0; h--) {
                    strncpy(live_sim_data.lap_history[h], 
                            live_sim_data.lap_history[h - 1], 
                            sizeof(live_sim_data.lap_history[h]) - 1);
                    live_sim_data.lap_history[h][sizeof(live_sim_data.lap_history[h]) - 1] = '\0';
                }
                strncpy(live_sim_data.lap_history[0], trig.s_ptr, sizeof(live_sim_data.lap_history[0]) - 1);
                live_sim_data.lap_history[0][sizeof(live_sim_data.lap_history[0]) - 1] = '\0';
            }
        }
        strncpy(rt.last_str, trig.s_ptr, sizeof(rt.last_str) - 1);
    }
    return triggered;
}
// End of eval_alert_string

// -----------------------------------------------------------------------------
// Function: eval_alert_threshold_duration
// Description: Monitors an integer channel and triggers an alert if the value
//              persists at or above a threshold for continuous milliseconds.
//              Dynamically calculates redline threshold when monitoring RPM.
// -----------------------------------------------------------------------------
static bool eval_alert_threshold_duration(const DashAlertDef& alert, const AlertTriggerDef& trig, AlertRuntimeState& rt, uint32_t now, char* val_buf, size_t buf_size) {
    if (!trig.i_ptr) return false;
    float cur = (float)(*trig.i_ptr);

    // Resolve trip threshold: If monitoring RPM, dynamically base it on car redline
    float trip_point = trig.delta_or_threshold;
    if (trig.i_ptr == &live_sim_data.rpm) {
        float dynamic_redline = live_sim_data.get_conventional_max_rpm();
        if (trip_point <= 0.0f) {
            // Negative/zero value acts as an offset from redline (e.g. -200 means redline - 200 RPM)
            trip_point = dynamic_redline + trip_point;
        } else if (trip_point < 1.0f) {
            // Fractional percentage (e.g. 0.95 = 95% of redline)
            trip_point = dynamic_redline * trip_point;
        }
    }

    if (cur >= trip_point && cur > 500.0f) {
        if (rt.condition_start_time == 0) {
            rt.condition_start_time = now;
        } else if (!rt.latched && (now - rt.condition_start_time >= (uint32_t)trig.duration_ms)) {
            rt.latched = true;
            snprintf(val_buf, buf_size, trig.format ? trig.format : "%d", (int)cur);
            return true;
        }
    } else {
        rt.condition_start_time = 0;
        rt.latched              = false;
    }
    return false;
}
// End of eval_alert_threshold_duration
// -----------------------------------------------------------------------------
// Function: check_declarative_alerts
// Description: Evaluates alert blueprint trigger rules every frame.
// -----------------------------------------------------------------------------
void check_declarative_alerts() {
    uint32_t now = millis();

    for (size_t i = 0; i < NUM_ALERT_BLUEPRINTS; i++) {
        DashAlertDef& alert         = alert_blueprints[i];
        const AlertTriggerDef& trig = alert.trigger;
        AlertRuntimeState& rt       = alert.runtime;

        bool triggered = false;
        char val_buf[32] = {0};

       switch (trig.type) {
            case ALERT_TRIGGER_DELTA_FLOAT:
                triggered = eval_alert_delta_float(trig, rt, val_buf, sizeof(val_buf));
                break;
            case ALERT_TRIGGER_CHANGE_INT:
                triggered = eval_alert_change_int(trig, rt, val_buf, sizeof(val_buf));
                break;
            case ALERT_TRIGGER_BOOL_ON:
                triggered = eval_alert_bool_on(trig, rt, val_buf, sizeof(val_buf));
                break;
            case ALERT_TRIGGER_STRING:
                triggered = eval_alert_string(alert, trig, rt, val_buf, sizeof(val_buf));
                break;
            case ALERT_TRIGGER_THRESHOLD_DURATION:
                triggered = eval_alert_threshold_duration(alert, trig, rt, now, val_buf, sizeof(val_buf));
                break;
        }

        if (triggered) {
            active_alert.active    = true;
            active_alert.rendered  = false;
            strncpy(active_alert.title, alert.title, sizeof(active_alert.title) - 1);
            strncpy(active_alert.value_str, val_buf, sizeof(active_alert.value_str) - 1);
            active_alert.color     = alert.color;
            active_alert.expiry_ms = now + alert.display_duration_ms;
        }
    }
}
// End of check_declarative_alerts

// =============================================================================
// INDIVIDUAL DECLARATIVE ELEMENT RENDER DISPATCHERS
// =============================================================================

static void update_element_speed(DashElementDef& el, int x, int y, int w, int h) {
    float raw_spd = live_sim_data.speed_kmh;
    if (app_config.use_mph) raw_spd *= 0.621371f;

    int int_spd = (int)roundf(raw_spd);
    if (int_spd < 0) int_spd = 0;
    if (int_spd > 999) int_spd = 999;

    char val_buf[16];
    snprintf(val_buf, sizeof(val_buf), 
             (el.source.format && strlen(el.source.format) > 0) ? el.source.format : "%03d", 
             int_spd);

    if (el.runtime.needs_full_draw || strcmp(val_buf, el.runtime.text_cache) != 0) {
        strncpy(el.runtime.text_cache, val_buf, sizeof(el.runtime.text_cache) - 1);
        el.runtime.needs_full_draw = false;

        bool has_label = (el.label && strlen(el.label) > 0);
        int clear_y    = has_label ? (y + 24) : (y + 4);
        int clear_h    = has_label ? (h - 28) : (h - 8);

        gfx.fillRect(x + 3, clear_y, w - 6, clear_h, DASH_CARD_BG);
        draw_data_value_autofit(x, y, w, h, val_buf, el.color, has_label);
    }
}

static void update_element_rpm_text(DashElementDef& el, int x, int y, int w, int h) {
    int cur_rpm = live_sim_data.rpm;
    if (cur_rpm < 0)     cur_rpm = 0;
    if (cur_rpm > 99999) cur_rpm = 99999;

    char val_buf[16];
    snprintf(val_buf, sizeof(val_buf), 
             (el.source.format && strlen(el.source.format) > 0) ? el.source.format : "%05d", 
             cur_rpm);

    if (el.runtime.needs_full_draw || strcmp(val_buf, el.runtime.text_cache) != 0) {
        strncpy(el.runtime.text_cache, val_buf, sizeof(el.runtime.text_cache) - 1);
        el.runtime.needs_full_draw = false;

        bool has_label = (el.label && strlen(el.label) > 0);
        int clear_y    = has_label ? (y + 24) : (y + 4);
        int clear_h    = has_label ? (h - 28) : (h - 8);

        gfx.fillRect(x + 3, clear_y, w - 6, clear_h, DASH_CARD_BG);
        draw_data_value_autofit(x, y, w, h, val_buf, el.color, has_label);
    }
}

static void update_element_text_or_number(DashElementDef& el, int x, int y, int w, int h) {
    char val_buf[32] = {0};

    if (el.source.type == SRC_FLOAT && el.source.f_ptr) {
        snprintf(val_buf, sizeof(val_buf), el.source.format ? el.source.format : "%.1f", *el.source.f_ptr);
    } else if (el.source.type == SRC_INT && el.source.i_ptr) {
        snprintf(val_buf, sizeof(val_buf), el.source.format ? el.source.format : "%d", *el.source.i_ptr);
    } else if (el.source.type == SRC_STRING && el.source.s_ptr) {
        strncpy(val_buf, el.source.s_ptr, sizeof(val_buf) - 1);
    }

    if (el.runtime.needs_full_draw || strcmp(val_buf, el.runtime.text_cache) != 0) {
        strncpy(el.runtime.text_cache, val_buf, sizeof(el.runtime.text_cache) - 1);
        el.runtime.needs_full_draw = false;

        bool has_label = (el.label && strlen(el.label) > 0);
        int clear_y    = has_label ? (y + 24) : (y + 4);
        int clear_h    = has_label ? (h - 28) : (h - 8);

        gfx.fillRect(x + 3, clear_y, w - 6, clear_h, DASH_CARD_BG);
        draw_data_value_autofit(x, y, w, h, val_buf, el.color, has_label);
    }
}
static void update_element_lightbox(DashElementDef& el, int x, int y, int w, int h) {
    bool is_on = false;
    if (el.source.type == SRC_BOOL && el.source.b_ptr) {
        is_on = *el.source.b_ptr;
    } else if (el.source.type == SRC_STRING && el.source.s_ptr) {
        is_on = (strlen(el.source.s_ptr) > 0 && 
                 strcmp(el.source.s_ptr, "None") != 0 && 
                 strcmp(el.source.s_ptr, "0") != 0 &&
                 strcasecmp(el.source.s_ptr, "false") != 0);
    } else if (el.source.type == SRC_INT && el.source.i_ptr) {
        is_on = (*el.source.i_ptr != 0);
    } else if (el.source.type == SRC_FLOAT && el.source.f_ptr) {
        is_on = (*el.source.f_ptr != 0.0f);
    }

    const char* active_text = (el.source.type == SRC_STRING && el.source.s_ptr) ? el.source.s_ptr : (el.label ? el.label : "");
    const char* target_text = is_on ? active_text : "";

    if (el.runtime.needs_full_draw || is_on != el.runtime.last_avail || strcmp(target_text, el.runtime.text_cache) != 0) {
        el.runtime.last_avail      = is_on;
        el.runtime.needs_full_draw = false;
        strncpy(el.runtime.text_cache, target_text, sizeof(el.runtime.text_cache) - 1);

        uint16_t box_color  = is_on ? el.color : DASH_CARD_BG;
        uint16_t txt_color  = is_on ? TFT_BLACK : DASH_TEXT_SLATE;
        uint16_t border_col = is_on ? TFT_WHITE : DASH_CARD_BORDER;

        gfx.fillRoundRect(x + 1, y + 1, w - 2, h - 2, 6, box_color);
        gfx.drawRoundRect(x + 1, y + 1, w - 4, h - 2, 6, border_col);

        if (is_on && strlen(target_text) > 0) {
            gfx.setFont(DASH_FONT_LABEL);
            gfx.setTextSize(0.75f);
            gfx.setTextDatum(MC_DATUM);
            gfx.setTextColor(txt_color, box_color);
            gfx.drawString(target_text, x + (w / 2), y + (h / 2));
        }
    }
}
// End of update_element_lightbox

static void update_element_icon(DashElementDef& el, int x, int y, int w, int h) {
    bool is_active = (el.source.b_ptr && *el.source.b_ptr);
    bool is_avail  = (!el.source.available_ptr || *el.source.available_ptr);

    if (el.runtime.needs_full_draw || is_active != el.runtime.last_avail) {
        el.runtime.last_avail      = is_active;
        el.runtime.needs_full_draw = false;

        uint16_t col = (!is_avail) ? DASH_TEXT_SLATE : (is_active ? el.color : 0x2124);
        gfx.fillRoundRect(x + 1, y + 1, w - 2, h - 2, 6, DASH_CARD_BG);
        gfx.drawRoundRect(x + 3, y + 3, w - 6, h - 6, 6, col);

        if (el.label) {
            gfx.setFont(DASH_FONT_LABEL);
            gfx.setTextSize(0.70f);
            gfx.setTextDatum(MC_DATUM);
            gfx.setTextColor(col, DASH_CARD_BG);
            gfx.drawString(el.label, x + (w / 2), y + (h / 2));
        }
    }
}
// End of update_element_icon

static void update_element_bar(DashElementDef& el) {
    // If bound to RPM, dynamically adjust max_val to match car class
    if (el.label && strstr(el.label, "RPM") != nullptr) {
        float conv_max = live_sim_data.get_conventional_max_rpm();
        if (el.source.max_val != conv_max) {
            el.source.max_val = conv_max;
            el.runtime.needs_full_draw = true;
        }
    }

    float raw_v = 0.0f;
    if (el.source.type == SRC_INT && el.source.i_ptr) {
        raw_v = (float)(*el.source.i_ptr);
    } else if (el.source.type == SRC_FLOAT && el.source.f_ptr) {
        raw_v = *el.source.f_ptr;
    }

    if (el.runtime.needs_full_draw) {
        el.runtime.eased_val    = raw_v;
        el.runtime.last_raw_val = -9999.0f;
    }

    float alpha = (raw_v > el.runtime.eased_val) ? 0.35f : 0.22f;
    el.runtime.eased_val += (raw_v - el.runtime.eased_val) * alpha;

    float span = el.source.max_val - el.source.min_val;
    if (span > 0.0f && fabsf(raw_v - el.runtime.eased_val) < (span * 0.003f)) {
        el.runtime.eased_val = raw_v;
    }

    float pct = (span > 0.0f) ? (el.runtime.eased_val - el.source.min_val) / span : 0.0f;
    pct = constrain(pct, 0.0f, 1.0f);

    bool moved = fabsf(pct - el.runtime.last_raw_val) >= 0.0025f;

    if (el.runtime.needs_full_draw || moved) {
        bool full = el.runtime.needs_full_draw;
        el.runtime.last_raw_val    = pct;
        el.runtime.needs_full_draw = false;
        draw_bar_element(el, el.runtime.eased_val, full);
    }
}
// End of update_element_bar

static void update_element_dial(DashElementDef& el) {
    // If bound to RPM, dynamically adjust max_val to match car class
    if (el.label && strstr(el.label, "RPM") != nullptr) {
        float conv_max = live_sim_data.get_conventional_max_rpm();
        if (el.source.max_val != conv_max) {
            el.source.max_val = conv_max;
            el.runtime.needs_full_draw = true;
        }
    }

    float raw_v = 0.0f;
    if (el.source.type == SRC_INT && el.source.i_ptr) {
        raw_v = (float)(*el.source.i_ptr);
    } else if (el.source.type == SRC_FLOAT && el.source.f_ptr) {
        raw_v = *el.source.f_ptr;
    }

    if (el.label && strstr(el.label, "SPEED") != nullptr && app_config.use_mph) {
        raw_v *= 0.621371f;
    }

    if (el.runtime.needs_full_draw) {
        el.runtime.eased_val    = raw_v;
        el.runtime.last_raw_val = -9999.0f;
    }

    float alpha = (raw_v > el.runtime.eased_val) ? 0.35f : 0.22f;
    el.runtime.eased_val += (raw_v - el.runtime.eased_val) * alpha;
    if (fabsf(raw_v - el.runtime.eased_val) < 0.25f) el.runtime.eased_val = raw_v;

    if (el.runtime.needs_full_draw || fabsf(el.runtime.eased_val - el.runtime.last_raw_val) >= 0.5f) {
        bool full = el.runtime.needs_full_draw;
        el.runtime.last_raw_val    = el.runtime.eased_val;
        el.runtime.needs_full_draw = false;
        draw_dial_element(el, el.runtime.eased_val, full);
    }
}
// End of update_element_dial

static void update_element_radar(DashElementDef& el) {
    float dist = fabsf(live_sim_data.opponent_behind_dist);
    bool in_range = (dist > 0.5f && dist < 35.0f);
    bool dist_changed = (fabsf(dist - el.runtime.last_raw_val) >= 0.1f);
    bool spotter_changed = (live_sim_data.spotter_left > 0) || (live_sim_data.spotter_right > 0);

    if (el.runtime.needs_full_draw || dist_changed || spotter_changed || in_range) {
        bool full = el.runtime.needs_full_draw;
        el.runtime.last_raw_val    = dist;
        el.runtime.needs_full_draw = false;
        draw_radar_element(el, full);
    }
}
// End of update_element_radar

// =============================================================================
// MAIN UI ENGINE LOOP CONTROLLERS
// =============================================================================

void render_declarative_page_static() {
    gfx.fillScreen(DASH_BG_GLOBAL);

    for (int col = 0; col <= GRID_COLS; col++) {
        int px = GRID_MARGIN_LEFT + col * GRID_CELL_WIDTH;
        gfx.drawFastVLine(px, GRID_MARGIN_TOP, GRID_ROWS * GRID_CELL_HEIGHT, DASH_GRID_LINE);
    }
    for (int row = 0; row <= GRID_ROWS; row++) {
        int py = GRID_MARGIN_TOP + row * GRID_CELL_HEIGHT;
        gfx.drawFastHLine(GRID_MARGIN_LEFT, py, GRID_COLS * GRID_CELL_WIDTH, DASH_GRID_LINE);
    }

    for (size_t i = 0; i < NUM_DASH_ELEMENTS; i++) {
        DashElementDef& el = dash_elements[i];
        if (el.page == active_page) {
            if (el.type != TYPE_LIGHTBOX && el.type != TYPE_ICON && 
                el.type != TYPE_DIAL && el.type != TYPE_RADAR) {
                draw_element_skeleton(el);
            }
            el.runtime.needs_full_draw    = true;
            el.runtime.text_cache[0]      = '\0';
            el.runtime.last_raw_val       = -9999.0f;
            el.runtime.eased_val          = 0.0f;
            el.runtime.last_needle_angle  = -1.0f;
            el.runtime.last_avail         = false;
        }
    }
}
// End of render_declarative_page_static

void update_declarative_ui() {
    check_screen_sleep();
    if (screen_sleeping || page_menu_open || modal_popup_open) return;

    if (live_sim_data.requested_page >= 0) {
        int target = live_sim_data.requested_page;
        live_sim_data.requested_page = -1;

        if (target != (int)active_page && target >= 0 && target < (int)TOTAL_PAGES) {
            bool page_exists = false;
            for (size_t e = 0; e < NUM_DASH_ELEMENTS; e++) {
                if ((int)dash_elements[e].page == target) {
                    page_exists = true;
                    break;
                }
            }

            if (page_exists) {
                menu_action_switch_page((DashboardPage)target);
                return;
            }
        }
    }

    uint32_t now = millis();

    if (active_alert.active) {
        if (now > active_alert.expiry_ms) {
            active_alert.active   = false;
            active_alert.rendered = false;
            render_declarative_page_static();
        } else if (!active_alert.rendered) {
            active_alert.rendered = true;

            const int card_w = 480;
            const int card_h = 240;
            const int card_x = (800 - card_w) / 2;
            const int card_y = (480 - card_h) / 2;

            gfx.fillRoundRect(card_x, card_y, card_w, card_h, 14, active_alert.color);
            gfx.fillRoundRect(card_x + 4, card_y + 4, card_w - 8, card_h - 8, 12, DASH_BG_GLOBAL);

            gfx.setFont(DASH_FONT_LABEL);
            gfx.setTextSize(0.9f);
            gfx.setTextDatum(MC_DATUM);
            gfx.setTextColor(DASH_TEXT_SLATE, DASH_BG_GLOBAL);
            gfx.drawString(active_alert.title, card_x + (card_w / 2), card_y + 45);

            gfx.setFont(DASH_FONT_HERO);
            gfx.setTextSize(1.15f);
            gfx.setTextColor(active_alert.color, DASH_BG_GLOBAL);
            gfx.drawString(active_alert.value_str, card_x + (card_w / 2), card_y + 130);
        }
        return;
    }

    gfx.startWrite();

    for (size_t i = 0; i < NUM_DASH_ELEMENTS; i++) {
        DashElementDef& el = dash_elements[i];
        if (el.page != active_page) continue;

        int x, y, w, h;
        grid_to_pixels(el.row, el.col, el.row_span, el.col_span, x, y, w, h);

        switch (el.type) {
            case TYPE_SPEED:
                update_element_speed(el, x, y, w, h);
                break;
            case TYPE_RPM_TEXT:
                update_element_rpm_text(el, x, y, w, h);
                break;
            case TYPE_REALNUMBER:
            case TYPE_INTEGER:
            case TYPE_TEXT:
            case TYPE_TIME:
                update_element_text_or_number(el, x, y, w, h);
                break;
            case TYPE_LIGHTBOX:
                update_element_lightbox(el, x, y, w, h);
                break;
            case TYPE_ICON:
                update_element_icon(el, x, y, w, h);
                break;
            case TYPE_BAR:
                update_element_bar(el);
                break;
            case TYPE_DIAL:
                update_element_dial(el);
                break;
            case TYPE_RADAR:
                update_element_radar(el);
                break;
        }
    }

    gfx.endWrite();
}
// End of update_declarative_ui