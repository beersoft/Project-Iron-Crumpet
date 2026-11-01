#pragma once



extern LGFX gfx;

// Exact palette: Deep Royal Blue and Ale Amgber
#ifndef COL_DEEP_ROYAL
#define COL_DEEP_ROYAL gfx.color565(16, 68, 168)  // #1044A8 (Deep Royal Blue)
#define COL_ALE_AMBER  gfx.color565(212, 112, 8)  // #D47008 (Ale Amber)
#define COL_CORP_BG    gfx.color565(255, 255, 255) // Pure White
#endif

// =============================================================================
// Fullscreen / Splash Logo (Centered on screen)
// =============================================================================
inline void draw_beersoft_corporate_logo(int cx, int cy, float scale = 1.0f) {
    int total_w = (int)(620.0f * scale);
    int total_h = (int)(210.0f * scale);
    int x0 = cx - (total_w / 2);
    int y0 = cy - (total_h / 2);

    // Clean white canvas backing
    gfx.fillRect(x0 - 20, y0 - 15, total_w + 40, total_h + 30, COL_CORP_BG);

    #define SX(v) (x0 + (int)((v) * scale))
    #define SY(v) (y0 + (int)((v) * scale))

    // --- Foam Outline & Crown Clouds ---
    gfx.fillCircle(SX(40),  SY(42), (int)(18 * scale), COL_DEEP_ROYAL);
    gfx.fillCircle(SX(75),  SY(32), (int)(24 * scale), COL_DEEP_ROYAL);
    gfx.fillCircle(SX(112), SY(42), (int)(18 * scale), COL_DEEP_ROYAL);
    gfx.fillCircle(SX(128), SY(52), (int)(12 * scale), COL_DEEP_ROYAL);

    // Hollow out the foam cloud center
    gfx.fillCircle(SX(40),  SY(42), (int)(9 * scale),  COL_CORP_BG);
    gfx.fillCircle(SX(75),  SY(32), (int)(15 * scale), COL_CORP_BG);
    gfx.fillCircle(SX(112), SY(42), (int)(9 * scale),  COL_CORP_BG);
    gfx.fillCircle(SX(128), SY(52), (int)(6 * scale),  COL_CORP_BG);

    // Floating foam arc bubbles above
    gfx.fillCircle(SX(52), SY(40), (int)(12 * scale), COL_DEEP_ROYAL);
    gfx.fillCircle(SX(52), SY(40), (int)(7 * scale),  COL_CORP_BG);
    gfx.fillCircle(SX(85), SY(34), (int)(10 * scale), COL_DEEP_ROYAL);
    gfx.fillCircle(SX(85), SY(34), (int)(5 * scale),  COL_CORP_BG);

    // --- Foam Rim & Hanging Drip ---
    gfx.fillRect(SX(14), SY(62), (int)(128 * scale), (int)(9 * scale), COL_DEEP_ROYAL);
    gfx.fillRect(SX(96), SY(71), (int)(14 * scale), (int)(32 * scale), COL_DEEP_ROYAL);
    gfx.fillCircle(SX(103), SY(103), (int)(7 * scale), COL_DEEP_ROYAL);

    // --- Liquid Core Polygon (Ale Amber) ---
    gfx.fillTriangle(SX(28), SY(78), SX(128), SY(78), SX(38), SY(190), COL_ALE_AMBER);
    gfx.fillTriangle(SX(128), SY(78), SX(116), SY(190), SX(38), SY(190), COL_ALE_AMBER);

    // Left shadow depth inside glass
    const uint16_t COL_AMBER_SHADOW = gfx.color565(160, 70, 0);
    gfx.fillTriangle(SX(28), SY(78), SX(48), SY(78), SX(38), SY(190), COL_AMBER_SHADOW);

    // --- White Cutout Channels ---
    gfx.fillRect(SX(39), SY(78), (int)(10 * scale), (int)(112 * scale), COL_CORP_BG);
    gfx.fillRect(SX(56), SY(96), (int)(10 * scale), (int)(42 * scale), COL_CORP_BG);
    gfx.fillRect(SX(56), SY(140), (int)(10 * scale), (int)(14 * scale), COL_CORP_BG);

    // --- Outer Tapered Glass Shell ---
    for (int t = 0; t < (int)(10 * scale); t++) {
        gfx.drawLine(SX(20 + t), SY(71), SX(32 + t), SY(205), COL_DEEP_ROYAL);
    }
    for (int t = 0; t < (int)(8 * scale); t++) {
        gfx.drawLine(SX(134 + t), SY(71), SX(120 + t), SY(195), COL_DEEP_ROYAL);
    }

    // Bottom Base Blocks
    gfx.fillRect(SX(32), SY(195), (int)(58 * scale), (int)(12 * scale), COL_DEEP_ROYAL);
    gfx.fillRect(SX(98), SY(195), (int)(30 * scale), (int)(12 * scale), COL_ALE_AMBER);

    #undef SX
    #undef SY

    // --- Typography ---
    int text_x = x0 + (int)(185.0f * scale);
    int ty_beer = y0 + (int)(45.0f * scale);
    int ty_inte = y0 + (int)(125.0f * scale);

    gfx.setTextDatum(TL_DATUM);

    // "BEERSOFT"
    gfx.setFont(&NotoSans_Bold_64);
    gfx.setTextSize(1);
    gfx.setTextColor(COL_DEEP_ROYAL, COL_CORP_BG);
    gfx.drawString("BEERSOFT", text_x, ty_beer);

    // "INTERACTIVE"
    gfx.setFont(&NotoSans_Bold_48);
    gfx.setTextSize(1);
    gfx.setTextColor(COL_ALE_AMBER, COL_CORP_BG);
    gfx.drawString("INTERACTIVE", text_x, ty_inte);
}

// =============================================================================
// Corner Badge Widget (Fits sprDeltaBox: 286x72 or Assist Tray)
// =============================================================================
inline void draw_beersoft_interactive_badge(LovyanGFX* target, int x, int y, int w, int h) {
    const uint16_t COL_WHITE = TFT_WHITE;

    // 1. Solid White Base Card with Rounded Border
    target->fillRoundRect(x, y, w, h, 8, COL_WHITE);
    target->drawRoundRect(x, y, w, h, 8, target->color565(190, 195, 205));

    // 2. Dynamic Emblem Scaling
    float scale = (float)(h - 8) / 150.0f;
    if (scale > 0.42f) scale = 0.42f;

    int emblem_x = x + 12;
    int emblem_y = y + (h / 2) - (int)(105.0f * scale);

    #define ASX(v) (emblem_x + (int)((v) * scale))
    #define ASY(v) (emblem_y + (int)((v) * scale))

    // Foam crown
    target->fillCircle(ASX(40),  ASY(42), (int)(18 * scale), COL_DEEP_ROYAL);
    target->fillCircle(ASX(75),  ASY(32), (int)(24 * scale), COL_DEEP_ROYAL);
    target->fillCircle(ASX(112), ASY(42), (int)(18 * scale), COL_DEEP_ROYAL);
    target->fillCircle(ASX(128), ASY(52), (int)(12 * scale), COL_DEEP_ROYAL);

    // Foam inner cutouts
    target->fillCircle(ASX(40),  ASY(42), (int)(9 * scale),  COL_WHITE);
    target->fillCircle(ASX(75),  ASY(32), (int)(15 * scale), COL_WHITE);
    target->fillCircle(ASX(112), ASY(42), (int)(9 * scale),  COL_WHITE);
    target->fillCircle(ASX(128), ASY(52), (int)(6 * scale),  COL_WHITE);

    // Foam bubbles
    target->fillCircle(ASX(52), ASY(40), (int)(12 * scale), COL_DEEP_ROYAL);
    target->fillCircle(ASX(52), ASY(40), (int)(7 * scale),  COL_WHITE);
    target->fillCircle(ASX(85), ASY(34), (int)(10 * scale), COL_DEEP_ROYAL);
    target->fillCircle(ASX(85), ASY(34), (int)(5 * scale),  COL_WHITE);

    // Rim & Drip
    target->fillRect(ASX(14), ASY(62), (int)(128 * scale), (int)(9 * scale), COL_DEEP_ROYAL);
    target->fillRect(ASX(96), ASY(71), (int)(14 * scale), (int)(32 * scale), COL_DEEP_ROYAL);
    target->fillCircle(ASX(103), ASY(103), (int)(7 * scale), COL_DEEP_ROYAL);

    // Ale Amber liquid core
    target->fillTriangle(ASX(28), ASY(78), ASX(128), ASY(78), ASX(38), ASY(190), COL_ALE_AMBER);
    target->fillTriangle(ASX(128), ASY(78), ASX(116), ASY(190), ASX(38), ASY(190), COL_ALE_AMBER);

    // Inner shadow
    const uint16_t COL_AMBER_SHADOW = target->color565(160, 70, 0);
    target->fillTriangle(ASX(28), ASY(78), ASX(48), ASY(78), ASX(38), ASY(190), COL_AMBER_SHADOW);

    // White reflection slits
    target->fillRect(ASX(39), ASY(78), (int)(10 * scale), (int)(112 * scale), COL_WHITE);
    target->fillRect(ASX(56), ASY(96), (int)(10 * scale), (int)(42 * scale), COL_WHITE);
    target->fillRect(ASX(56), ASY(140), (int)(10 * scale), (int)(14 * scale), COL_WHITE);

    // Outer tapered glass borders
    for (int t = 0; t < (int)(10 * scale); t++) {
        target->drawLine(ASX(20 + t), ASY(71), ASX(32 + t), ASY(205), COL_DEEP_ROYAL);
    }
    for (int t = 0; t < (int)(8 * scale); t++) {
        target->drawLine(ASX(134 + t), ASY(71), ASX(120 + t), ASY(195), COL_DEEP_ROYAL);
    }

    // Foot base blocks
    target->fillRect(ASX(32), ASY(195), (int)(58 * scale), (int)(12 * scale), COL_DEEP_ROYAL);
    target->fillRect(ASX(98), ASY(195), (int)(30 * scale), (int)(12 * scale), COL_ALE_AMBER);

    #undef ASX
    #undef ASY

    // 3. Compact Typography using NotoSans_Bold_28 & NotoSans_Bold_20
    int text_x = emblem_x + (int)(155.0f * scale) + 12;
    target->setTextDatum(TL_DATUM);

    // "BEERSOFT" in Deep Royal Blue
    target->setFont(&NotoSans_Bold_28);
    target->setTextSize(1);
    target->setTextColor(COL_DEEP_ROYAL, COL_WHITE);
    target->drawString("BEERSOFT", text_x, y + 10);

    // "INTERACTIVE" in Ale Amber
    target->setFont(&NotoSans_Bold_20);
    target->setTextSize(1);
    target->setTextColor(COL_ALE_AMBER, COL_WHITE);
    target->drawString("INTERACTIVE", text_x, y + 42);
}