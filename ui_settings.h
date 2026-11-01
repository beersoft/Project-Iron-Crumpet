#ifndef UI_SETTINGS_H
#define UI_SETTINGS_H

#include <Arduino.h>

// JetBrains Mono VLW font paths stored in LittleFS
#define FONT_JB_MONO_12  "/JetBrainsMono_12.vlw"
#define FONT_JB_MONO_16  "/JetBrainsMono_16.vlw"
#define FONT_JB_MONO_20  "/JetBrainsMono_20.vlw"
#define FONT_JB_MONO_28  "/JetBrainsMono_28.vlw"
#define FONT_JB_MONO_48  "/JetBrainsMono_48.vlw"
#define FONT_JB_MONO_72  "/JetBrainsMono_72.vlw"

// UI Global Settings & Theme Defaults
struct UISettings {
    uint16_t primaryColor   = 0xFFFF; // White
    uint16_t secondaryColor = 0x07E0; // Green
    uint16_t backgroundColor= 0x0000; // Black
    const char* activeFont  = FONT_JB_MONO_20;
};

extern UISettings uiSettings;

#endif // UI_SETTINGS_H