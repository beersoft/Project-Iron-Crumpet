// =============================================================================
// PROJECT: Iron Crumpet Dashboard
// FILE: Iron-Crumpet.ino
// DESCRIPTION:
//   Main hardware orchestration, peripheral bus initialisation, FreeRTOS task
//   dispatch, and primary execution loop for Project Iron Crumpet.
//   Includes a high-contrast Beersoft Interactive splash screen on boot.
// =============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>

#include "LovyanGFX_Driver.h"
#include "config_manager.h"
#include "simhub_client_ws.h"
#include "UI.h"

// -----------------------------------------------------------------------------
// Hardware Display Driver Instance
// -----------------------------------------------------------------------------
LGFX gfx;

// Dedicated SPI bus instance for SD card operations
SPIClass sd_spi = SPIClass(FSPI);

// -----------------------------------------------------------------------------
// Class: MemoryFontWrapper
// Description: Streams raw TTF bytes from PROGMEM flash to LovyanGFX on the fly.
// -----------------------------------------------------------------------------
class MemoryFontWrapper : public lgfx::v1::DataWrapper {
    const uint8_t* data;
    size_t len;
    size_t pos;
public:
    MemoryFontWrapper(const uint8_t* ptr, size_t length) : data(ptr), len(length), pos(0) {}

    bool open(const char* path) override { pos = 0; return true; }
    void close() override {}
    
    int read(uint8_t *buf, uint32_t length) override {
        if (pos >= len) return 0;
        if (pos + length > len) length = len - pos;
        memcpy_P(buf, data + pos, length);
        pos += length;
        return length;
    }

    void skip(int32_t offset) override {
        pos += offset;
        if (pos > len) pos = len;
        if (pos < 0) pos = 0;
    }

    bool seek(uint32_t offset) override {
        if (offset > len) return false;
        pos = offset;
        return true;
    }

    int32_t tell(void) override { return pos; }
};

// -----------------------------------------------------------------------------
// Function: setup_custom_fonts
// Description: Loads in-memory PROGMEM TrueType fonts into LovyanGFX for
//              smooth, anti-aliased font rendering via memory stream.
// -----------------------------------------------------------------------------
void setup_custom_fonts() {
    Serial.println("[FONT] Loading TTF font from flash via memory wrapper...");

    static MemoryFontWrapper font_wrapper(Orbitron_Regular_ttf, Orbitron_Regular_ttf_len);
    bool success = gfx.loadFont(&font_wrapper, lgfx::v1::IFont::font_type_t::ft_ttf);

    if (success) {
        Serial.println("[FONT] SUCCESS: TTF loaded into LovyanGFX from flash memory!");
    } else {
        Serial.println("[FONT] ERROR: Failed to parse TTF from flash memory wrapper!");
    }
}
// End of setup_custom_fonts

// -----------------------------------------------------------------------------
// Function: show_boot_splash
// Description: Renders a clean white splash screen featuring a 600x400 brand box
//              with guaranteed boundary containment for all logo elements & typography.
// -----------------------------------------------------------------------------
static void show_boot_splash() {
    const uint16_t COL_CANVAS_BG   = TFT_WHITE;
    const uint16_t COL_CARD_BG     = TFT_WHITE;
    const uint16_t COL_CARD_BORDER = gfx.color565(180, 185, 195);
    const uint16_t COL_ROYAL_BLUE  = gfx.color565(16, 68, 168);  // Deep Royal Blue
    const uint16_t COL_AMBER       = gfx.color565(212, 112, 8);   // Warm Ale Amber
    const uint16_t COL_TEXT_DARK   = gfx.color565(20, 24, 35);    // Deep Slate/Black

    // 1. Full Screen Pure White Canvas
    gfx.fillScreen(COL_CANVAS_BG);

    // 2. Centered 600x400 Container Box
    const int box_w = 600;
    const int box_h = 400;
    const int box_x = (800 - box_w) / 2;
    const int box_y = (480 - box_h) / 2;

    gfx.fillRoundRect(box_x, box_y, box_w, box_h, 12, COL_CARD_BG);
    gfx.drawRoundRect(box_x, box_y, box_w, box_h, 12, COL_CARD_BORDER);
    gfx.drawRoundRect(box_x + 1, box_y + 1, box_w - 2, box_h - 2, 11, COL_ROYAL_BLUE);

    // 3. Vector Pint Glass Emblem
    const float scale  = 0.62f;
    const int emblem_x = box_x + 28;
    const int emblem_y = box_y + 42;

    #define BSX(v) (emblem_x + (int)((v) * scale))
    #define BSY(v) (emblem_y + (int)((v) * scale))

    // Foam Head
    gfx.fillCircle(BSX(40),  BSY(42), (int)(18 * scale), COL_ROYAL_BLUE);
    gfx.fillCircle(BSX(75),  BSY(32), (int)(24 * scale), COL_ROYAL_BLUE);
    gfx.fillCircle(BSX(112), BSY(42), (int)(18 * scale), COL_ROYAL_BLUE);
    gfx.fillCircle(BSX(128), BSY(52), (int)(12 * scale), COL_ROYAL_BLUE);

    gfx.fillCircle(BSX(40),  BSY(42), (int)(9  * scale), TFT_WHITE);
    gfx.fillCircle(BSX(75),  BSY(32), (int)(15 * scale), TFT_WHITE);
    gfx.fillCircle(BSX(112), BSY(42), (int)(9  * scale), TFT_WHITE);
    gfx.fillCircle(BSX(128), BSY(52), (int)(6  * scale), TFT_WHITE);

    // Glass Rim & Base
    gfx.fillRect(BSX(14), BSY(62),  (int)(128 * scale), (int)(10 * scale), COL_ROYAL_BLUE);
    gfx.fillRect(BSX(45), BSY(255), (int)(65  * scale), (int)(16 * scale), COL_ROYAL_BLUE);

    // Liquid Body (Amber Ale Core)
    gfx.fillTriangle(BSX(26),  BSY(72),  BSX(130), BSY(72),  BSX(45),  BSY(255), COL_AMBER);
    gfx.fillTriangle(BSX(130), BSY(72),  BSX(110), BSY(255), BSX(45),  BSY(255), COL_AMBER);

    // Glass Outer Walls
    for (int t = 0; t < (int)(8 * scale); t++) {
        gfx.drawLine(BSX(22 + t),  BSY(70), BSX(45 + t),  BSY(255), COL_ROYAL_BLUE);
        gfx.drawLine(BSX(134 - t), BSY(70), BSX(110 - t), BSY(255), COL_ROYAL_BLUE);
    }

    #undef BSX
    #undef BSY

    // 4. Bounded Typography Column
    const int text_x     = box_x + 145;
    const int max_text_w = box_w - (text_x - box_x) - 25;
    gfx.setTextDatum(TL_DATUM);

    // "BEERSOFT's"
    const char* str_beer = "BEERSOFT's";
    gfx.setFont(nullptr);
    float s_beer = 0.95f;
    gfx.setTextSize(s_beer);
    while (gfx.textWidth(str_beer) > max_text_w && s_beer > 0.5f) {
        s_beer -= 0.05f;
        gfx.setTextSize(s_beer);
    }
    gfx.setTextColor(COL_ROYAL_BLUE, COL_CARD_BG);
    gfx.drawString(str_beer, text_x, box_y + 40);

    // Divider Line
    gfx.drawFastHLine(text_x, box_y + 138, max_text_w, COL_CARD_BORDER);

    // "IRON CRUMPET"
    const char* str_crumpet = "IRON CRUMPET";
    gfx.setFont(nullptr);
    float s_crumpet = 1.05f;
    gfx.setTextSize(s_crumpet);
    while (gfx.textWidth(str_crumpet) > max_text_w && s_crumpet > 0.5f) {
        s_crumpet -= 0.05f;
        gfx.setTextSize(s_crumpet);
    }
    gfx.setTextColor(COL_TEXT_DARK, COL_CARD_BG);
    gfx.drawString(str_crumpet, text_x, box_y + 155);

    // Subtitle Tag
    const char* str_sub = "SIMHUB TELEMETRY DASHBOARD";
    gfx.setFont(DASH_FONT_LABEL);
    float s_sub = 0.58f;
    gfx.setTextSize(s_sub);
    while (gfx.textWidth(str_sub) > max_text_w && s_sub > 0.35f) {
        s_sub -= 0.04f;
        gfx.setTextSize(s_sub);
    }
    gfx.setTextColor(gfx.color565(120, 125, 140), COL_CARD_BG);
    gfx.drawString(str_sub, text_x, box_y + 225);

    // 5. Progress Bar
    const int bar_w = box_w - 70;
    const int bar_h = 10;
    const int bar_x = box_x + 35;
    const int bar_y = box_y + box_h - 48;

    gfx.fillRoundRect(bar_x, bar_y, bar_w, bar_h, 5, gfx.color565(235, 238, 245));
    gfx.drawRoundRect(bar_x, bar_y, bar_w, bar_h, 5, COL_CARD_BORDER);

    for (int p = 0; p <= 100; p += 4) {
        int fill = (bar_w - 4) * p / 100;
        gfx.fillRoundRect(bar_x + 2, bar_y + 2, fill, bar_h - 4, 3, COL_ROYAL_BLUE);
        delay(12);
    }

    delay(350);
}
// End of show_boot_splash

// -----------------------------------------------------------------------------
// Helper: connect_wifi
// Connects to configured Wi-Fi network with visual serial progress
// -----------------------------------------------------------------------------
static void connect_wifi() {
    Serial.printf("[WIFI] Connecting to SSID: '%s' ...\n", app_config.wifi_ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(app_config.wifi_ssid, app_config.wifi_password);

    uint32_t start_ms = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start_ms < 12000) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WIFI] Connected! Assigned IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("[WIFI] Connection timed out. SimHub task will retry in background.");
    }
}
// End of connect_wifi

// =============================================================================
// ARDUINO SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n\n========================================");
    Serial.println("  BEERSOFT INTERACTIVE: IRON CRUMPET    ");
    Serial.println("========================================");

    // 1. Initialise Hardware Power Rails for Selected Board
#if defined(BOARD_ELECROW_DIS02170A)
    Serial.println("[SETUP] Powering rails for CrowPanel Advance 7.0\"...");
    init_crowpanel_advance_power();
#elif defined(BOARD_ELECROW_DIS07050H)
    Serial.println("[SETUP] Powering rails for CrowPanel Basic 5.0\"...");
    init_crowpanel_dis07050h_power();
#endif

    // 2. Initialise LovyanGFX Display & Touch Digitizer
    Serial.println("[SETUP] Initialising LovyanGFX display panel...");
    gfx.init();
    gfx.setRotation(0);
    gfx.setBrightness(255);

    // 3. Load Embedded TrueType Fonts via Memory Wrapper
    setup_custom_fonts(); 

    // 4. Display Beersoft Interactive Boot Screen
    show_boot_splash();

    // 5. Initialise SD Card Peripheral Bus
    Serial.println("[SETUP] Checking SD card bus...");
    bool sd_mounted = init_board_sd_card();
    if (sd_mounted) {
        Serial.println("[SETUP] SD Card mounted successfully.");
    } else {
        Serial.println("[SETUP] SD Card not detected. Proceeding with internal NVS.");
    }

    // 6. Load or Create Configuration
    ConfigStatus cfg_status = load_or_create_config("/beersoft_dash.cfg");
    Serial.printf("[SETUP] Config loaded status: %d\n", (int)cfg_status);

    current_brightness_idx = app_config.brightness_idx;
    current_timeout_idx    = app_config.idle_timeout_idx;
    active_page            = (DashboardPage)app_config.default_page;

    gfx.setBrightness(brightness_levels[current_brightness_idx]);

    if (app_config.game_override_idx > 0) {
        strncpy(live_sim_data.game_name, 
                game_override_names[app_config.game_override_idx], 
                sizeof(live_sim_data.game_name) - 1);
    }
    if (app_config.class_override_idx > 0) {
        strncpy(live_sim_data.car_class, 
                class_override_names[app_config.class_override_idx], 
                sizeof(live_sim_data.car_class) - 1);
    }

    // 7. Connect to Network
    connect_wifi();

    // 8. Initialise Telemetry State & Start WebSocket Client Task on Core 0
    live_sim_data.reset_to_zero();
    SimHubWs::start_simhub_ws_client(2, 0);

    // 9. First Static Paint of Active Dashboard View
    Serial.println("[SETUP] Rendering initial dashboard layout...");
    render_declarative_page_static();
    Serial.println("[SETUP] Iron Crumpet pipeline active.\n");
}
// End of setup

// =============================================================================
// ARDUINO MAIN LOOP
// =============================================================================
void loop() {
    // 1. Ingest Capacitive Touch Events
    uint16_t touch_x = 0;
    uint16_t touch_y = 0;
    bool is_touched = gfx.getTouch(&touch_x, &touch_y);

    handle_ui_touch(touch_x, touch_y, is_touched);

    // 2. Evaluate Declarative Pop-up Alert Triggers
    check_declarative_alerts();

    // 3. High-Frequency UI Paint Pass
    update_declarative_ui();

    // Yield brief tick for FreeRTOS scheduler hygiene
    vTaskDelay(pdMS_TO_TICKS(1));
}
// End of loop