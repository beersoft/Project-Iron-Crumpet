#include <Arduino.h>
#include <LittleFS.h>
#include "LovyanGFX_Driver.h"
#include "ui_settings.h"
#include "simhub_client.h"
#include "telemetry_smoother.h"

// Instantiate display driver object
LGFX tft;
UISettings uiSettings;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[Init] Starting Project Iron Crumpet...");

    // Initialize LittleFS storage for font files
    if (!LittleFS.begin(true)) {
        Serial.println("[Error] LittleFS Mount Failed!");
        return;
    }
    Serial.println("[Init] LittleFS mounted successfully.");

    // Initialize display
    tft.init();
    tft.setRotation(1); // Adjust rotation as needed for your dash layout
    tft.fillScreen(uiSettings.backgroundColor);

    // Load default JetBrains Mono font from LittleFS storage
    // Note: LovyanGFX expects the filesystem object second: tft.loadFont(path, fs)
    if (tft.loadFont(FONT_JB_MONO_20, LittleFS)) {
        Serial.println("[Init] JetBrains Mono 20 loaded successfully.");
    } else {
        Serial.println("[Warning] Failed to load JetBrains Mono 20 font file!");
    }

    // Render startup confirmation text
    tft.setTextColor(uiSettings.primaryColor, uiSettings.backgroundColor);
    tft.setCursor(10, 10);
    tft.print("IRON CRUMPET READY");
    
    // Clean up font memory when switching fonts later if needed:
    // tft.unloadFont();
}

void loop() {
    // Main loop logic for telemetry updates & rendering
    // Add your runtime screen refresh calls here
}