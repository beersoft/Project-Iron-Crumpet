// ui_settings.h
#pragma once

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "fonts/Orbitron_Bold_12.h"
#include "fonts/Orbitron_Regular_16.h"
#include "fonts/Orbitron_Bold_20.h"
#include "fonts/Orbitron_Bold_36.h"
#include "fonts/Orbitron_Bold_72.h"
#include "fonts/NotoSans_Bold_64.h"
#include "fonts/NotoSans_Bold_48.h"
#include "fonts/NotoSans_Bold_28.h" 
#include "fonts/NotoSans_Bold_20.h" 
// -----------------------------------------------------------------------------
// Global UI Theme Colors
// -----------------------------------------------------------------------------
#define DASH_BG_GLOBAL      0x0A2B // 0x52AA  // Dark gunmetal slate global background
#define DASH_CARD_BG        0x2945
#define DASH_CARD_BORDER    0x3186  // Subtle card boundary
#define DASH_CARD_SUBTLE    0x2124  // Inner dividers
#define DASH_TEXT_SLATE     0x7BEF  // Slate for inactive/unavailable states

// -----------------------------------------------------------------------------
// Global Typography & Font Sizing
// -----------------------------------------------------------------------------
#define DASH_FONT_LABEL_SIZE        0.80f

// Sizes optimized for LovyanGFX Font7 (7-Segment LED)
#define DASH_FONT_NUM_SM_SIZE       0.80f   // Sectors, bottom-row secondary
#define DASH_FONT_NUM_MD_SIZE       0.80f  // Lap times, delta, pressures
#define DASH_FONT_NUM_LG_SIZE       0.80f   // RPM, secondary hero numbers
#define DASH_FONT_NUM_HERO_SIZE     0.80f   // Primary Speed / Gear readouts

#define DASH_FONT_DATA_SM_SIZE    0.80f
#define DASH_FONT_DATA_MD_SIZE     0.80f
#define DASH_FONT_DATA_LG_SIZE     0.80f
#define DASH_FONT_DATA_HERO_SIZE   3.80f

#define DASH_FONT_LABEL      &Orbitron_Bold_36 
#define DASH_FONT_DATA       &Orbitron_Bold_36
#define DASH_FONT_DATA_NUM   &Orbitron_Bold_36
#define DASH_FONT_HERO_NUM   &Orbitron_Bold_72
// -----------------------------------------------------------------------------
// UI Grid Geometry (800x480: 12 Columns, 6 Rows between RPM bar & Assist Tray)
// -----------------------------------------------------------------------------

#define GRID_ORIGIN_X       18   // Centered horizontally with 18px side margins
#define GRID_ORIGIN_Y       38   // Starts below RPM bar
#define GRID_GAP            4    // Updated from 8 to 4px

#define GRID_COL_WIDTH      60   // Updated from 56 to 60px
#define GRID_ROW_HEIGHT     55   // Updated from 50 to 55px



