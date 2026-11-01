#include "UI.h"
#include "icons.h"

// =============================================================================
// Quick-Action Control Menu Blueprints
// =============================================================================

// Page 1: Dashboard View Selection (4 Views + Next Page + Close)
MenuItemDef menu_page1_items[] = {
    { "1. RACE VIEW",     TFT_CYAN,        ACTION_SWITCH_PAGE,       PAGE_RACE },
    { "2. DRIVE VIEW",    TFT_GREEN,       ACTION_SWITCH_PAGE,       PAGE_DRIVE },
    { "3. TYRES MATRIX",  TFT_ORANGE,      ACTION_SWITCH_PAGE,       PAGE_TYRES },
    { "3. F1 COCKPIT",    0x9FD0,          ACTION_SWITCH_PAGE, PAGE_F1 },
    { "4. TIMING / DELTA",TFT_MAGENTA,     ACTION_SWITCH_PAGE,       PAGE_TIMING },
    { "SETTINGS >>",      TFT_YELLOW,      ACTION_MENU_PAGE_2,       PAGE_RACE },
    { "CANCEL / CLOSE",   DASH_TEXT_SLATE, ACTION_CLOSE_MENU,        PAGE_RACE }
};
const size_t NUM_MENU_PAGE1_ITEMS = sizeof(menu_page1_items) / sizeof(menu_page1_items[0]);

// Page 2: Settings & Hardware Utilities
MenuItemDef menu_page2_items[] = {
    { "GAME: AUTO",       TFT_CYAN,        ACTION_CYCLE_GAME_OVERRIDE,  PAGE_RACE },
    { "CLASS: AUTO",      TFT_MAGENTA,     ACTION_CYCLE_CLASS_OVERRIDE, PAGE_RACE },
    { "BRIGHTNESS",       TFT_WHITE,       ACTION_CYCLE_BRIGHTNESS,     PAGE_RACE },
    { "IDLE TIMEOUT",     TFT_SKYBLUE,     ACTION_CYCLE_IDLE_TIMEOUT,   PAGE_RACE },
    { "SPEED UNIT",       TFT_GREEN,       ACTION_TOGGLE_SPEED_UNIT,    PAGE_RACE },
    { "DEMO SWEEP",       TFT_YELLOW,      ACTION_DEMO_SWEEP,           PAGE_RACE },
    { "NET & SERVER INFO",TFT_ORANGE,      ACTION_SHOW_CONFIG_INFO,     PAGE_RACE },
    { "<< BACK",          DASH_TEXT_SLATE, ACTION_MENU_PAGE_1,          PAGE_RACE }
};
const size_t NUM_MENU_PAGE2_ITEMS = sizeof(menu_page2_items) / sizeof(menu_page2_items[0]);

// =============================================================================
// Declarative Layout Blueprints
// =============================================================================
DashElementDef dash_elements[] = {
    // =========================================================================
    // PAGE 1: RACE
    // =========================================================================
    { PAGE_RACE, TYPE_LIGHTBOX,   1, 1, 1, 4, "",         { SRC_STRING, { .s_ptr = live_sim_data.flag_name },   "%s" },                   DASH_CARD_BG },
    { PAGE_RACE, TYPE_BAR,        1, 5, 1, 8, "RPM",      { SRC_INT,    { .i_ptr = &live_sim_data.rpm },        "%d", 0.0f, 8500.0f },    DASH_TEXT_SLATE },
    { PAGE_RACE, TYPE_REALNUMBER, 1, 13, 1, 4, "DELTA",    { SRC_FLOAT,  { .f_ptr = &live_sim_data.delta_to_best }, "%+.2f" },             TFT_GREEN },
    
    // Speed + Gear + RPM Digits
   // { PAGE_RACE, TYPE_SPEED,      2, 1, 4, 6, "",         { SRC_FLOAT,  { .f_ptr = &live_sim_data.speed_kmh }, "%3.0f" },                 DASH_TEXT_SLATE },
    { PAGE_RACE, TYPE_SPEED,      4, 1, 3, 6, "Speed",         { SRC_FLOAT, { .f_ptr = &live_sim_data.speed_kmh }, "%03d" }, DASH_TEXT_SLATE },
    { PAGE_RACE, TYPE_TEXT,       2, 7, 7, 4, "",         { SRC_STRING, { .s_ptr = live_sim_data.gear },        "%s" },                   TFT_DARKGREEN },
 //   { PAGE_RACE, TYPE_RPM_TEXT,    2, 11, 4, 6, "",         { SRC_INT,    { .i_ptr = &live_sim_data.rpm },         "%d" },                   DASH_TEXT_SLATE },
      { PAGE_RACE, TYPE_RPM_TEXT, 4, 11, 3, 6, "RPM", { SRC_INT,   { .i_ptr = &live_sim_data.rpm },       "%05d" }, DASH_TEXT_SLATE },
    // Driver Assists & Timing Strip
    { PAGE_RACE, TYPE_INTEGER,    10, 1, 1, 1, "LAP",      { SRC_INT,    { .i_ptr = &live_sim_data.current_lap },   "%d" },                 DASH_TEXT_SLATE },
    { PAGE_RACE, TYPE_INTEGER,    10, 2, 1, 1, "POS",      { SRC_INT,    { .i_ptr = &live_sim_data.position },      "%d" },                 DASH_TEXT_SLATE },
    { PAGE_RACE, TYPE_INTEGER,    10, 3, 1, 1, "ABS",      { SRC_INT,    { .i_ptr = &live_sim_data.abs_level },     "%d" },                 DASH_TEXT_SLATE },
    { PAGE_RACE, TYPE_INTEGER,    10, 4, 1, 1, "TC",       { SRC_INT,    { .i_ptr = &live_sim_data.tc_level },      "%d" },                 DASH_TEXT_SLATE },
    { PAGE_RACE, TYPE_INTEGER,    10, 5, 1, 1, "MAP",      { SRC_INT,    { .i_ptr = &live_sim_data.engine_map },    "%d" },                 DASH_TEXT_SLATE },
    { PAGE_RACE, TYPE_INTEGER,    10, 6, 1, 1, "ERS",      { SRC_INT,    { .i_ptr = &live_sim_data.ers_mode },      "%d" },                 DASH_TEXT_SLATE },
    { PAGE_RACE, TYPE_REALNUMBER, 10, 7, 1, 2, "BIAS",     { SRC_FLOAT,  { .f_ptr = &live_sim_data.brake_bias }, "%.1f" },              DASH_TEXT_SLATE },
    { PAGE_RACE, TYPE_TIME,       9, 1, 1, 6, "",  { SRC_STRING, { .s_ptr = live_sim_data.current_lap_time }, "%s" },             TFT_WHITE },

    // 3-State Icons: PIT, ABS, TC, LIGHTS
    { PAGE_RACE, TYPE_ICON,       9, 9,  2, 2, "PIT",     { SRC_BOOL,   { .b_ptr = &live_sim_data.pit_limiter_on }, "%s", 0, 0, ICON_ISO_0086, &live_sim_data.pit_limiter_available }, TFT_BLUE },
    { PAGE_RACE, TYPE_ICON,       9, 11, 2, 2, "ABS",     { SRC_BOOL,   { .b_ptr = &live_sim_data.abs_active },     "%s", 0.0f, 0.0f, ICON_ISO_0239, &live_sim_data.abs_available },    TFT_ORANGE },
    { PAGE_RACE, TYPE_ICON,       9, 13, 2, 2, "TC",      { SRC_BOOL,   { .b_ptr = &live_sim_data.tc_active },      "%s", 0.0f, 0.0f, ICON_ISO_2630, &live_sim_data.tc_available },     TFT_BLUE },
    { PAGE_RACE, TYPE_ICON,       9, 15, 2, 2, "LIGHTS",  { SRC_BOOL,   { .b_ptr = &live_sim_data.headlights_on },  "%s", 0.0f, 0.0f, ICON_ISO_0082, &live_sim_data.lights_available }, TFT_GREEN },

    // =========================================================================
    // PAGE 2: DRIVE (Cockpit Layout with Radial Dials)
    // =========================================================================
    { PAGE_DRIVE, TYPE_TEXT,       1, 1,  1, 3, "GEAR",     { SRC_STRING, { .s_ptr = live_sim_data.gear },      "%s" },                 TFT_YELLOW },
    { PAGE_DRIVE, TYPE_TIME,       1, 8,  1, 5, "LAST LAP", { SRC_STRING, { .s_ptr = live_sim_data.last_lap_time }, "%s" },           TFT_WHITE },
    { PAGE_DRIVE, TYPE_DIAL,       2, 1,  4, 6, "RPM",      { SRC_INT,    { .i_ptr = &live_sim_data.rpm },        "%d", 0.0f, 8500.0f },  TFT_MAGENTA },
    { PAGE_DRIVE, TYPE_TEXT,       5, 5,  1, 2, "",         { SRC_STRING, { .s_ptr = live_sim_data.gear },        "%s" },                   TFT_DARKGREEN },
    { PAGE_DRIVE, TYPE_DIAL,       2, 7,  4, 6, "SPEED",    { SRC_FLOAT,  { .f_ptr = &live_sim_data.speed_kmh }, "%d", 0.0f, 320.0f },   TFT_CYAN },
    { PAGE_DRIVE, TYPE_SPEED,      5, 10, 1, 3, "",         { SRC_FLOAT,  { .f_ptr = &live_sim_data.speed_kmh }, "%3.0f" },                 DASH_TEXT_SLATE },
    { PAGE_DRIVE, TYPE_BAR,        1, 4,  1, 4, "RPM",      { SRC_INT,    { .i_ptr = &live_sim_data.rpm },        "%d", 0.0f, 8500.0f },  TFT_MAGENTA },

    { PAGE_DRIVE, TYPE_REALNUMBER, 6, 1,  1, 3, "BIAS",     { SRC_FLOAT,  { .f_ptr = &live_sim_data.brake_bias }, "%.1f" },              TFT_WHITE },
    { PAGE_DRIVE, TYPE_LIGHTBOX,   6, 9,  1, 1, "PIT",      { SRC_BOOL,   { .b_ptr = &live_sim_data.pit_limiter_on }, "%s" },            TFT_BLUE },
    { PAGE_DRIVE, TYPE_LIGHTBOX,   6, 4,  1, 5, "FLAG",     { SRC_STRING, { .s_ptr = live_sim_data.flag_name }, "%s" },                 TFT_YELLOW },

    // 3-State Icons: ABS, TC, LIGHTS
    { PAGE_DRIVE, TYPE_ICON,       6, 10, 1, 1, "ABS",      { SRC_BOOL,   { .b_ptr = &live_sim_data.abs_active },    "%s", 0.0f, 0.0f, ICON_ISO_0239, &live_sim_data.abs_available },    TFT_ORANGE },
    { PAGE_DRIVE, TYPE_ICON,       6, 11, 1, 1, "TC",       { SRC_BOOL,   { .b_ptr = &live_sim_data.tc_active },     "%s", 0.0f, 0.0f, ICON_ISO_2630, &live_sim_data.tc_available },     TFT_BLUE },
    { PAGE_DRIVE, TYPE_ICON,       6, 12, 1, 1, "LIGHTS",   { SRC_BOOL,   { .b_ptr = &live_sim_data.headlights_on }, "%s", 0.0f, 0.0f, ICON_ISO_0082, &live_sim_data.lights_available }, TFT_GREEN },

    // =========================================================================
    // PAGE 3: TYRES
    // =========================================================================
    { PAGE_TYRES, TYPE_REALNUMBER, 1, 1,  1, 3, "FL TEMP",  { SRC_FLOAT,  { .f_ptr = &live_sim_data.fl.temp_c },   "%.1f C" },           TFT_CYAN },
    { PAGE_TYRES, TYPE_REALNUMBER, 1, 4,  1, 3, "FL PRES",  { SRC_FLOAT,  { .f_ptr = &live_sim_data.fl.pressure }, "%.1f psi" },         TFT_WHITE },
    { PAGE_TYRES, TYPE_REALNUMBER, 1, 7,  1, 3, "FR PRES",  { SRC_FLOAT,  { .f_ptr = &live_sim_data.fr.pressure }, "%.1f psi" },         TFT_WHITE },
    { PAGE_TYRES, TYPE_REALNUMBER, 1, 10, 1, 3, "FR TEMP",  { SRC_FLOAT,  { .f_ptr = &live_sim_data.fr.temp_c },   "%.1f C" },           TFT_CYAN },
    { PAGE_TYRES, TYPE_REALNUMBER, 3, 1,  1, 3, "FL BRK",   { SRC_FLOAT,  { .f_ptr = &live_sim_data.fl.brake_c },  "%.0f C" },           TFT_ORANGE },
    { PAGE_TYRES, TYPE_REALNUMBER, 3, 10, 1, 3, "FR BRK",   { SRC_FLOAT,  { .f_ptr = &live_sim_data.fr.brake_c },  "%.0f C" },           TFT_ORANGE },
    { PAGE_TYRES, TYPE_REALNUMBER, 5, 1,  1, 3, "RL TEMP",  { SRC_FLOAT,  { .f_ptr = &live_sim_data.rl.temp_c },   "%.1f C" },           TFT_CYAN },
    { PAGE_TYRES, TYPE_REALNUMBER, 5, 4,  1, 3, "RL PRES",  { SRC_FLOAT,  { .f_ptr = &live_sim_data.rl.pressure }, "%.1f psi" },         TFT_WHITE },
    { PAGE_TYRES, TYPE_REALNUMBER, 5, 7,  1, 3, "RR PRES",  { SRC_FLOAT,  { .f_ptr = &live_sim_data.rr.pressure }, "%.1f psi" },         TFT_WHITE },
    { PAGE_TYRES, TYPE_REALNUMBER, 5, 10, 1, 3, "RR TEMP",  { SRC_FLOAT,  { .f_ptr = &live_sim_data.rr.temp_c },   "%.1f C" },           TFT_CYAN },

    // =========================================================================
    // PAGE 4: TELEMETRY MATRIX (24 Cards)
    // =========================================================================
    { PAGE_TELEMETRY, TYPE_REALNUMBER, 1, 1,  1, 3, "H2O T",   { SRC_FLOAT,  { .f_ptr = &live_sim_data.water_temperature }, "%.0f C" },    TFT_CYAN },
    { PAGE_TELEMETRY, TYPE_REALNUMBER, 1, 4,  1, 3, "OIL T",   { SRC_FLOAT,  { .f_ptr = &live_sim_data.oil_temperature },   "%.0f C" },    TFT_ORANGE },
    { PAGE_TELEMETRY, TYPE_REALNUMBER, 1, 7,  1, 3, "OIL P",   { SRC_FLOAT,  { .f_ptr = &live_sim_data.oil_pressure },      "%.1f" },      TFT_WHITE },
    { PAGE_TELEMETRY, TYPE_REALNUMBER, 1, 10, 1, 3, "FUEL P",  { SRC_FLOAT,  { .f_ptr = &live_sim_data.fuel_pressure },     "%.1f" },      TFT_WHITE },

    { PAGE_TELEMETRY, TYPE_REALNUMBER, 2, 1,  1, 3, "AIR T",   { SRC_FLOAT,  { .f_ptr = &live_sim_data.air_temperature },  "%.1f C" },    TFT_CYAN },
    { PAGE_TELEMETRY, TYPE_REALNUMBER, 2, 4,  1, 3, "ROAD T",  { SRC_FLOAT,  { .f_ptr = &live_sim_data.road_temperature }, "%.1f C" },    TFT_YELLOW },
    { PAGE_TELEMETRY, TYPE_TEXT,       2, 7,  1, 3, "SESS",    { SRC_STRING, { .s_ptr = live_sim_data.session_type },      "%s" },        TFT_WHITE },
    { PAGE_TELEMETRY, TYPE_INTEGER,    2, 10, 1, 3, "TLAPS",   { SRC_INT,    { .i_ptr = &live_sim_data.total_laps },        "%d" },        TFT_WHITE },

    { PAGE_TELEMETRY, TYPE_INTEGER,    3, 1,  1, 3, "TC SLIP", { SRC_INT,    { .i_ptr = &live_sim_data.tc_slip },          "%d" },        TFT_CYAN },
    { PAGE_TELEMETRY, TYPE_INTEGER,    3, 4,  1, 3, "TC CUT",  { SRC_INT,    { .i_ptr = &live_sim_data.tc_cut },           "%d" },        TFT_BLUE },
    { PAGE_TELEMETRY, TYPE_INTEGER,    3, 7,  1, 3, "MGU DEL", { SRC_INT,    { .i_ptr = &live_sim_data.mguk_delivery },    "%d" },        TFT_GREEN },
    { PAGE_TELEMETRY, TYPE_REALNUMBER, 3, 10, 1, 3, "ERS DEPL",{ SRC_FLOAT,  { .f_ptr = &live_sim_data.ers_lap_deployed }, "%.0f%%" },    TFT_YELLOW },

    { PAGE_TELEMETRY, TYPE_REALNUMBER, 4, 1,  1, 3, "RL BRK",  { SRC_FLOAT,  { .f_ptr = &live_sim_data.rl.brake_c },        "%.0f C" },    TFT_ORANGE },
    { PAGE_TELEMETRY, TYPE_REALNUMBER, 4, 4,  1, 3, "RR BRK",  { SRC_FLOAT,  { .f_ptr = &live_sim_data.rr.brake_c },        "%.0f C" },    TFT_ORANGE },
    { PAGE_TELEMETRY, TYPE_INTEGER,    4, 7,  1, 3, "MAX RPM", { SRC_INT,    { .i_ptr = &live_sim_data.max_rpm },           "%d" },        TFT_MAGENTA },
    { PAGE_TELEMETRY, TYPE_TIME,       4, 10, 1, 3, "BEST",    { SRC_STRING, { .s_ptr = live_sim_data.best_lap_time },     "%s" },        TFT_GREEN },

    { PAGE_TELEMETRY, TYPE_REALNUMBER, 5, 1,  1, 3, "AHEAD D", { SRC_FLOAT,  { .f_ptr = &live_sim_data.opponent_ahead_dist },"%.0fm" },    TFT_YELLOW },
    { PAGE_TELEMETRY, TYPE_REALNUMBER, 5, 4,  1, 3, "AHEAD G", { SRC_FLOAT,  { .f_ptr = &live_sim_data.opponent_ahead_gap }, "-%.2fs" },  TFT_YELLOW },
    { PAGE_TELEMETRY, TYPE_REALNUMBER, 5, 7,  1, 3, "CURVE",   { SRC_FLOAT,  { .f_ptr = &live_sim_data.road_curvature },    "%.3f" },      TFT_CYAN },
    { PAGE_TELEMETRY, TYPE_TIME,       5, 10, 1, 3, "ALL TIME",{ SRC_STRING, { .s_ptr = live_sim_data.all_time_best },     "%s" },        TFT_WHITE },

    { PAGE_TELEMETRY, TYPE_REALNUMBER, 6, 1,  1, 3, "BEHIND D",{ SRC_FLOAT,  { .f_ptr = &live_sim_data.opponent_behind_dist }, "%.0fm" },  TFT_ORANGE },
    { PAGE_TELEMETRY, TYPE_REALNUMBER, 6, 4,  1, 3, "BEHIND G",{ SRC_FLOAT,  { .f_ptr = &live_sim_data.opponent_behind_gap },  "+%.2fs" }, TFT_ORANGE },
    { PAGE_TELEMETRY, TYPE_REALNUMBER, 6, 7,  1, 3, "BEHIND S",{ SRC_FLOAT,  { .f_ptr = &live_sim_data.opponent_behind_speed }, "%.0f" },  TFT_CYAN },
    { PAGE_TELEMETRY, TYPE_REALNUMBER, 6, 10, 1, 3, "OFFSET",  { SRC_FLOAT,  { .f_ptr = &live_sim_data.opponent_behind_offset },"%+.1fm" }, TFT_WHITE },

    // =========================================================================
    // PAGE 5: TIMING (Live/Key Laps + History)
    // =========================================================================
    { PAGE_TIMING, TYPE_TIME, 1, 1, 2, 6, "CURRENT", 
      { SRC_STRING, { .s_ptr = live_sim_data.current_lap_time }, "%s" }, TFT_WHITE },
    { PAGE_TIMING, TYPE_TIME, 3, 1, 2, 6, "LAST LAP", 
      { SRC_STRING, { .s_ptr = live_sim_data.last_lap_time },    "%s" }, TFT_CYAN },
    { PAGE_TIMING, TYPE_TIME, 5, 1, 2, 6, "BEST LAP", 
      { SRC_STRING, { .s_ptr = live_sim_data.best_lap_time },    "%s" }, TFT_MAGENTA },

    { PAGE_TIMING, TYPE_TIME, 1, 7, 1, 6, "LAP -1", 
      { SRC_STRING, { .s_ptr = live_sim_data.lap_history[0] },   "%s" }, TFT_YELLOW },
    { PAGE_TIMING, TYPE_TIME, 2, 7, 1, 6, "LAP -2", 
      { SRC_STRING, { .s_ptr = live_sim_data.lap_history[1] },   "%s" }, TFT_DARKGRAY },
    { PAGE_TIMING, TYPE_TIME, 3, 7, 1, 6, "LAP -3", 
      { SRC_STRING, { .s_ptr = live_sim_data.lap_history[2] },   "%s" }, TFT_DARKGRAY },
    { PAGE_TIMING, TYPE_TIME, 4, 7, 1, 6, "LAP -4", 
      { SRC_STRING, { .s_ptr = live_sim_data.lap_history[3] },   "%s" }, TFT_DARKGRAY },
    { PAGE_TIMING, TYPE_TIME, 5, 7, 1, 6, "LAP -5", 
      { SRC_STRING, { .s_ptr = live_sim_data.lap_history[4] },   "%s" }, TFT_DARKGRAY },
    { PAGE_TIMING, TYPE_REALNUMBER, 6, 7, 1, 6, "DELTA", 
      { SRC_FLOAT,  { .f_ptr = &live_sim_data.delta_to_best },  "%+.3f" }, TFT_GREEN },

// =========================================================================
    // PAGE 6: F1 WHEEL DISPLAY
    // =========================================================================
    // Top Row: Speed (Left), Live Lap Time (Center), Gap / Delta (Right)
   
  
    // Middle Left: Lap Number & Front/Rear Left Tyre Temps
     { PAGE_F1, TYPE_SPEED,      1, 1, 1, 2, "MPH",     { SRC_FLOAT,  { .f_ptr = &live_sim_data.speed_kmh },        "%3.0f" },   TFT_WHITE },
    { PAGE_F1, TYPE_INTEGER,    2, 1, 1, 2, "LAP",     { SRC_INT,    { .i_ptr = &live_sim_data.current_lap },      "L%d" },     TFT_WHITE },
    { PAGE_F1, TYPE_REALNUMBER, 3, 1, 1, 2, "",        { SRC_FLOAT,  { .f_ptr = &live_sim_data.fl.temp_c },        "%.0f C" },  TFT_WHITE },
    { PAGE_F1, TYPE_REALNUMBER, 4, 1, 1, 2, "",        { SRC_FLOAT,  { .f_ptr = &live_sim_data.rl.temp_c },        "%.0f C" },  TFT_WHITE },

    // Center Hero: Giant Gear Readout
    { PAGE_F1, TYPE_TEXT,       3, 4, 2, 6, "",        { SRC_STRING, { .s_ptr = live_sim_data.gear },             "%s" },      TFT_WHITE },
  { PAGE_F1, TYPE_TIME,       2, 4, 1, 5, "",        { SRC_STRING, { .s_ptr = live_sim_data.current_lap_time }, "%s" },      TFT_WHITE },
    { PAGE_F1, TYPE_REALNUMBER, 1, 4, 1, 5, "",        { SRC_FLOAT,  { .f_ptr = &live_sim_data.delta_to_best },    "%+.2f" },   TFT_GREEN },

    // Middle Right: Position & Front/Rear Right Tyre Temps
    { PAGE_F1, TYPE_INTEGER,    2, 11, 1, 2, "POS",    { SRC_INT,    { .i_ptr = &live_sim_data.position },         "P%d" },     TFT_WHITE },
    { PAGE_F1, TYPE_REALNUMBER, 3, 11, 1, 2, "",       { SRC_FLOAT,  { .f_ptr = &live_sim_data.fr.temp_c },        "%.0f C" },  TFT_WHITE },
    { PAGE_F1, TYPE_REALNUMBER, 4, 11, 1, 2, "",       { SRC_FLOAT,  { .f_ptr = &live_sim_data.rr.temp_c },        "%.0f C" },  TFT_WHITE },

    // Bottom Rows 5-6: Full-Width ERS Battery Energy Bar
    { PAGE_F1, TYPE_BAR,        6, 1, 1, 12, "ERS",    { SRC_FLOAT,  { .f_ptr = &live_sim_data.ers_charge_pct },   "%.0f%%", 0.0f, 100.0f }, 0x9FD0 }



};
const size_t NUM_DASH_ELEMENTS = sizeof(dash_elements) / sizeof(dash_elements[0]);

// =============================================================================
// Declarative Alert Blueprints (Title, Binding, Color, Duration)
// =============================================================================
DashAlertDef alert_blueprints[] = {
    { "LAST LAP",           { ALERT_TRIGGER_STRING,             { .s_ptr = live_sim_data.last_lap_time },    "%s",       0.0f,    0.0f }, TFT_GREEN,  5000 },
    { "BRAKE BIAS",         { ALERT_TRIGGER_DELTA_FLOAT,        { .f_ptr = &live_sim_data.brake_bias },      "%.1f %%",  0.1f,    0.0f }, TFT_YELLOW, 1500 },
    { "TRACTION CONTROL",   { ALERT_TRIGGER_CHANGE_INT,         { .i_ptr = &live_sim_data.tc_level },        "LEVEL %d", 0.0f,    0.0f }, TFT_CYAN,   1500 },
    { "ABS SYSTEM",         { ALERT_TRIGGER_CHANGE_INT,         { .i_ptr = &live_sim_data.abs_level },       "LEVEL %d", 0.0f,    0.0f }, TFT_ORANGE, 1500 },
    { "ENGINE MAP",         { ALERT_TRIGGER_CHANGE_INT,         { .i_ptr = &live_sim_data.engine_map },      "MAP %d",   0.0f,    0.0f }, TFT_GREEN,  1500 },
    { "TC CUT",             { ALERT_TRIGGER_CHANGE_INT,         { .i_ptr = &live_sim_data.tc_cut },          "CUT %d",   0.0f,    0.0f }, TFT_CYAN,   1200 },
    { "TC SLIP",            { ALERT_TRIGGER_CHANGE_INT,         { .i_ptr = &live_sim_data.tc_slip },         "SLIP %d",  0.0f,    0.0f }, TFT_CYAN,   1200 },
   // Fast & Furious "Danger to Manifold" 3-second redline alert
    { "Warning!!!", { ALERT_TRIGGER_THRESHOLD_DURATION, { .i_ptr = &live_sim_data.rpm }, "Danger to Manifold", -150.0f, 3000.0f }, 0xDB24, 3000 },
    { "ERS MODE",           { ALERT_TRIGGER_CHANGE_INT,         { .i_ptr = &live_sim_data.ers_mode },        "MODE %d",  0.0f,    0.0f }, TFT_GREEN,  1500 },
    { "MGU-K HARVEST",      { ALERT_TRIGGER_CHANGE_INT,         { .i_ptr = &live_sim_data.mguk_recovery },   "LEVEL %d", 0.0f,    0.0f }, TFT_CYAN,   1500 }
};
const size_t NUM_ALERT_BLUEPRINTS = sizeof(alert_blueprints) / sizeof(alert_blueprints[0]);