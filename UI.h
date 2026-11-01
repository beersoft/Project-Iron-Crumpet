#pragma once

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "LovyanGFX_Driver.h"
#include "ui_settings.h"
#include "simhub_client_ws.h"
#include "config_manager.h"

extern LGFX gfx;
extern DashboardPage active_page;

// -----------------------------------------------------------------------------
// Unified Grid Engine (Dynamic GRID_ROWS x GRID_COLS across 800x480)
// -----------------------------------------------------------------------------
inline void grid_to_pixels(int row, int col, int row_span, int col_span, int& x, int& y, int& w, int& h) {
    int r = constrain(row, 1, GRID_ROWS) - 1;
    int c = constrain(col, 1, GRID_COLS) - 1;

    x = GRID_MARGIN_LEFT + (c * GRID_CELL_WIDTH);
    y = GRID_MARGIN_TOP  + (r * GRID_CELL_HEIGHT);
    w = col_span * GRID_CELL_WIDTH;
    h = row_span * GRID_CELL_HEIGHT;
}

// -----------------------------------------------------------------------------
// Declarative Element Enums & Data Structures
// -----------------------------------------------------------------------------
enum ElementType {
    TYPE_SPEED,
    TYPE_RPM_TEXT,
    TYPE_REALNUMBER,
    TYPE_INTEGER,
    TYPE_TEXT,
    TYPE_TIME,
    TYPE_LIGHTBOX,
    TYPE_ICON,
    TYPE_BAR,
    TYPE_DIAL,
    TYPE_RADAR
};

enum DataSourceType {
    SRC_FLOAT,
    SRC_INT,
    SRC_STRING,
    SRC_BOOL
};

struct DataSource {
    DataSourceType type;
    union {
        const float* f_ptr;
        const int*   i_ptr;
        const char*  s_ptr;
        const bool*  b_ptr;
    };
    const char*    format;
    float          min_val;
    float          max_val;
    const uint8_t* icon_data;
    const bool*    available_ptr;
};

struct ElementRuntimeState {
    char     text_cache[32]     = {0};
    uint16_t color_cache        = 0;
    float    last_raw_val       = -9999.0f;
    float    eased_val          = 0.0f;
    float    last_needle_angle  = -1.0f;
    bool     last_avail         = false;
    bool     needs_full_draw    = true;
};

struct DashElementDef {
    DashboardPage       page;
    ElementType         type;
    int                 row;
    int                 col;
    int                 row_span;
    int                 col_span;
    const char*         label;
    DataSource          source;
    uint16_t            color;
    ElementRuntimeState runtime;
};

// -----------------------------------------------------------------------------
// Declarative Pop-up Alert Structures
// -----------------------------------------------------------------------------
enum AlertTriggerType {
    ALERT_TRIGGER_DELTA_FLOAT,
    ALERT_TRIGGER_CHANGE_INT,
    ALERT_TRIGGER_BOOL_ON,
    ALERT_TRIGGER_STRING,
    ALERT_TRIGGER_THRESHOLD_DURATION
};

struct AlertTriggerDef {
    AlertTriggerType type;
    union {
        const float* f_ptr;
        const int*   i_ptr;
        const bool*  b_ptr;
        const char*  s_ptr;
    };
    const char* format;
    float       delta_or_threshold;
    float       duration_ms;
};

struct AlertRuntimeState {
    float    last_float           = -9999.0f;
    int      last_int             = -9999;
    bool     last_bool            = false;
    char     last_str[32]         = {0};
    uint32_t condition_start_time = 0;
    bool     latched              = false;
    bool     initialised          = false;
};

struct DashAlertDef {
    const char*       title;
    AlertTriggerDef   trigger;
    uint16_t          color;
    uint32_t          display_duration_ms;
    AlertRuntimeState runtime;
};

struct SettingAlert {
    bool     active;
    bool     rendered;
    char     title[32];
    char     value_str[32];
    uint16_t color;
    uint32_t expiry_ms;
};

// -----------------------------------------------------------------------------
// Control Menu Enums & Structures
// -----------------------------------------------------------------------------
enum MenuActionType {
    ACTION_SWITCH_PAGE,
    ACTION_MENU_PAGE_1,
    ACTION_MENU_PAGE_2,
    ACTION_CYCLE_BRIGHTNESS,
    ACTION_CYCLE_IDLE_TIMEOUT,
    ACTION_CYCLE_GAME_OVERRIDE,
    ACTION_CYCLE_CLASS_OVERRIDE,
    ACTION_DEMO_SWEEP,
    ACTION_TOGGLE_SPEED_UNIT,
    ACTION_SHOW_CONFIG_INFO,
    ACTION_SHOW_TERMINAL,
    ACTION_CLOSE_MENU
};

struct MenuItemDef {
    const char*    label;
    uint16_t       color;
    MenuActionType action_type;
    DashboardPage  target_page;
    int32_t        x, y, w, h;
};

// -----------------------------------------------------------------------------
// External Data Arrays & Runtime Objects
// -----------------------------------------------------------------------------
extern SettingAlert   active_alert;
extern DashElementDef dash_elements[];
extern const size_t   NUM_DASH_ELEMENTS;
extern DashAlertDef   alert_blueprints[];
extern const size_t   NUM_ALERT_BLUEPRINTS;

extern MenuItemDef    menu_page1_items[];
extern const size_t   NUM_MENU_PAGE1_ITEMS;
extern MenuItemDef    menu_page2_items[];
extern const size_t   NUM_MENU_PAGE2_ITEMS;

extern const uint8_t  brightness_levels[4];
extern uint8_t        current_brightness_idx;
extern const char*    timeout_labels[4];
extern uint8_t        current_timeout_idx;

// -----------------------------------------------------------------------------
// Core UI Prototypes
// -----------------------------------------------------------------------------
void setup_custom_fonts();
void check_screen_sleep();
void run_demo_sweep();
void toggle_speed_unit();
void render_page_menu(int highlight_idx = -1);
void render_config_info_popup();
void render_terminal_popup();
void handle_ui_touch(uint16_t tx, uint16_t ty, bool is_touched);
void check_declarative_alerts();
void render_declarative_page_static();
void update_declarative_ui();