// dash.ino - Rev 2.2.1
// Multi-Board SimHub Dashboard (Elecrow / Sunton)

#include "LovyanGFX_Driver.h"
#include <Wire.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <WiFi.h>

// Include your 1-bit XBM ISO icon headers from dash-ui/

#include "dash_ui.h"
#include "dash-ui/bitmap.h"
#include "dash-ui/logos.h"
#include "config_manager.h"
#include "simhub_client.h"



// Master UI Orchestrator (Load this early so DashboardPage, menu items, and functions are declared globally)


// Display & SPI Instances (declared in LovyanGFX_Driver.h)
LGFX gfx;
SPIClass sd_spi(FSPI);

// System synchronization & configuration
SemaphoreHandle_t sim_data_mutex = NULL;
SimTelemetry live_sim_data;
AppConfig app_config;
// Master UI Orchestrator
#include "dash_ui.h"

// -----------------------------------------------------------------------------
// Boot Logger Helpers (Serial Console Only to Keep Clean Splash)
// -----------------------------------------------------------------------------
void init_boot_screen() {
    gfx.fillScreen(TFT_WHITE);
    // Draw centered on 800x480 screen at 1:1 scale
    draw_beersoft_corporate_logo(400, 240, 1.0f);
}

void log_boot_status(const char* item, const char* status, uint16_t color) {
    Serial.printf("[BOOT] %-18s : %s\n", item, status);
}

void wake_stc8h_power_and_backlight() {
    Wire.begin(15, 16, 100000UL);
    delay(50);
    Wire.beginTransmission(0x30); Wire.write(0x18); Wire.endTransmission();
    delay(20);
    Wire.beginTransmission(0x30); Wire.write(0x10); Wire.endTransmission();
    delay(20);
    Wire.beginTransmission(0x30); Wire.write(0xCC);
    Wire.endTransmission();
    delay(100);
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n--- Beersoft SimHub Dashboard ---");

    Serial.printf("[SYSTEM] Free Heap:  %u bytes\n", ESP.getFreeHeap());
    Serial.printf("[SYSTEM] Free PSRAM: %u bytes\n", ESP.getFreePsram());

#if defined(BOARD_ELECROW_DIS07050H)
    // Basic 5.0" board: Release LCD reset and power via PCA9557 I2C expander
    init_crowpanel_dis07050h_power();
#elif defined(BOARD_ELECROW_DIS02050A)
    // Advance board only: Wake STC8H MCU coprocessor
    wake_stc8h_power_and_backlight();
#endif

    // Initialize display once
    if (!gfx.init()) {
        Serial.println("[ERR] gfx.init() failed! Halting to prevent panic.");
        while (true) {
            delay(1000);
        }
    }
    // Set to landscape orientation (1)
    gfx.setRotation(0);
    gfx.setBrightness(255);
    gfx.fillScreen(TFT_BLACK);

    init_boot_screen();
    log_boot_status("Hardware", "Display Ready", TFT_GREEN);

    // Initialise SD card at 4MHz to prevent bus starvation on 16MHz RGB panel[cite: 7]
    if (!init_board_sd_card(4000000)) {
        log_boot_status("SD Card", "Mount FAILED", TFT_RED);
    } else {
        log_boot_status("SD Card", "Mounted OK", TFT_GREEN);

        ConfigStatus st = load_or_create_config("/beersoft_dash.cfg");
        if (st == CONFIG_LOADED) {
            log_boot_status("Config File", "Loaded beersoft_dash.cfg", TFT_GREEN);
        } else if (st == CONFIG_CREATED_DEFAULT) {
            log_boot_status("Config File", "Created default template", TFT_ORANGE);
        } else {
            log_boot_status("Config File", "Read/Write Error", TFT_RED);
        }
    }

    log_boot_status("Wi-Fi SSID", app_config.wifi_ssid, TFT_YELLOW);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(app_config.wifi_ssid, app_config.wifi_password);

    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 25) {
        delay(400);
        timeout++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        char ip_str[48];
        snprintf(ip_str, sizeof(ip_str), "Connected (%s)", WiFi.localIP().toString().c_str());
        log_boot_status("Wi-Fi Status", ip_str, TFT_GREEN);
    } else {
        log_boot_status("Wi-Fi Status", "Connection FAILED", TFT_RED);
    }

    char ep_str[64];
    if (strlen(app_config.simhub_ip) > 0) {
        snprintf(ep_str, sizeof(ep_str), "%s:%d", app_config.simhub_ip, app_config.simhub_port);
    } else {
        snprintf(ep_str, sizeof(ep_str), "Autodetect Active (: %d)", app_config.simhub_port);
    }
    log_boot_status("SimHub Target", ep_str, TFT_CYAN);

    delay(1200);

    // Initialize all sprites and dashboard UI
    init_dashboard_ui();

    if (sim_data_mutex == NULL) {
        sim_data_mutex = xSemaphoreCreateMutex();
    }
    
    // Spawn network telemetry task on Core 0 with priority 2
    xTaskCreatePinnedToCore(
        simhub_poll_task,
        "simhub_task",
        8192,
        NULL,
        2,          // Priority elevated above idle/standard to avoid buffer stalls
        NULL,
        0           // Core 0 (Wi-Fi stack)
    );
}

void loop() {
    static uint32_t last_touch_ms = 0;
    static uint32_t logo_touch_start_ms = 0;
    static bool logo_long_pressed = false;

    uint16_t touchX, touchY;
    bool is_touched = gfx.getTouch(&touchX, &touchY);
    uint32_t now = millis();

    if (is_touched) {
        // ---------------------------------------------------------------------
        // 1. MENU IS ACTIVE: Process Button Selection or Outside Tap (8 Items)
        // ---------------------------------------------------------------------
        if (page_menu_open) {
            if (now - last_touch_ms > 250) {
                last_touch_ms = now;
                bool item_clicked = false;

                for (int i = 0; i < 8; i++) {
                    if (touchX >= page_menu_items[i].x && touchX <= (page_menu_items[i].x + page_menu_items[i].w) &&
                        touchY >= page_menu_items[i].y && touchY <= (page_menu_items[i].y + page_menu_items[i].h)) {
                        
                        // Highlight button briefly
                        render_page_menu(i);
                        delay(120);

                        page_menu_open = false;
                        switch_dashboard_page(page_menu_items[i].page);
                        item_clicked = true;
                        break;
                    }
                }

                // Tapped outside the buttons -> dismiss menu
                if (!item_clicked) {
                    page_menu_open = false;
                    render_active_page_static();
                }
            }
        }
        // ---------------------------------------------------------------------
        // 2. DISMISS ACTIVE ALERT ON TOUCH
        // ---------------------------------------------------------------------
        else if (active_alert.active) {
            if (now - last_touch_ms > 250) {
                last_touch_ms = now;
                active_alert.active = false;
                render_active_page_static();
                ui_cache.speed = -1;
                ui_cache.rpm   = -1;
                ui_cache.gear[0] = '\0';
                ui_cache.curr_lap[0] = '\0';
            }
        }
        // ---------------------------------------------------------------------
        // 3. TOUCHING THE LOGO AREA (X: 490..784, Y: 402..480)
        // ---------------------------------------------------------------------
        else if (touchX >= 490 && touchX <= 784 && touchY >= 402 && touchY <= 480) {
            if (logo_touch_start_ms == 0) {
                logo_touch_start_ms = now;
            } else if (!logo_long_pressed && (now - logo_touch_start_ms >= 2000)) {
                demo_test_mode = !demo_test_mode;
                logo_long_pressed = true;
                render_active_page_static();
            }
        }
        // ---------------------------------------------------------------------
        // 4. TOP HALF OF SCREEN (Y < 240): REGULAR SWIPE/PAGE CYCLE
        // ---------------------------------------------------------------------
        else if (touchY < 240) {
            logo_touch_start_ms = 0;
            logo_long_pressed = false;

            if (now - last_touch_ms > 350) {
                last_touch_ms = now;

                if (demo_test_mode) {
                    demo_test_mode = false;
                    render_active_page_static();
                } else {
                    int target_page;
                    if (touchX < 400) {
                        target_page = ((int)current_page - 1 + TOTAL_PAGES) % TOTAL_PAGES;
                    } else {
                        target_page = ((int)current_page + 1) % TOTAL_PAGES;
                    }
                    switch_dashboard_page((DashboardPage)target_page);
                }
            }
        }
        else {
            logo_touch_start_ms = 0;
            logo_long_pressed = false;
        }
    } else {
        if (logo_touch_start_ms > 0 && !logo_long_pressed && (now - logo_touch_start_ms < 1000)) {
            page_menu_open = true;
            render_page_menu();
        }

        logo_touch_start_ms = 0;
        logo_long_pressed = false;
    }

    // Refresh UI (~60 FPS)
    update_dashboard_ui();
    vTaskDelay(pdMS_TO_TICKS(16));
}