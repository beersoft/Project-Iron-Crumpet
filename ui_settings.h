#pragma once

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// =============================================================================
// Dashboard Pages Enum
// =============================================================================
enum DashboardPage {
    PAGE_RACE = 0,
    PAGE_DRIVE,
    PAGE_TYRES,
    PAGE_TIMING,
    PAGE_TELEMETRY,
    PAGE_F1,
    PAGE_DIALS,
    TOTAL_PAGES
};

// -----------------------------------------------------------------------------
// Global UI Theme Colors
// -----------------------------------------------------------------------------
#define DASH_BG_GLOBAL      0x0A2B  // Dark slate base
#define DASH_GRID_LINE      0x18E3  // Matrix background grid line
#define DASH_CARD_BG        0x1084  // Inner card background
#define DASH_CARD_BORDER    0x52AA  // Crisp card boundary
#define DASH_TEXT_SLATE     0x7BEF  // Inactive / secondary text

// -----------------------------------------------------------------------------
// Typography Presets (Native FreeSansBold Tiers)
// -----------------------------------------------------------------------------
#define DASH_FONT_LABEL      &fonts::FreeSansOblique12pt7b  // Slanted vector for card headers
#define DASH_FONT_DATA       nullptr                        // Uses smooth Orbitron TTF
#define DASH_FONT_HERO       nullptr                        // Uses smooth Orbitron TTF
#define DASH_FONT_NUM        nullptr                        // Uses smooth Orbitron TTF
// -----------------------------------------------------------------------------
// UI Full-Screen Grid Geometry (800x480: 16 Columns x 10 Rows @ 48x48)
// -----------------------------------------------------------------------------
#define GRID_MARGIN_LEFT    16
#define GRID_MARGIN_TOP     0
#define GRID_CELL_WIDTH     48
#define GRID_CELL_HEIGHT    48
#define GRID_COLS           16
#define GRID_ROWS           10