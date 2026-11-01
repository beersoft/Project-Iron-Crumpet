#pragma once

#include <Arduino.h>

// =============================================================================
// GLOBAL CONFIGURATION STRUCT
// Holds network credentials, server endpoints, and persistent UI preferences
// =============================================================================
struct AppConfig {
    char wifi_ssid[64]        = "FortBees 2.4";
    char wifi_password[64]    = "beersoft";
    char simhub_ip[32]        = "192.168.1.196";
    int  simhub_port          = 8888;
    char simhub_path[64]      = "/api/getgamedata";
    char full_simhub_url[160] = "";

    // Hardware & UI Preferences
    bool    use_mph           = false; // false = KPH, true = MPH
    uint8_t brightness_idx    = 0;     // 0 = 100%, 1 = 60%, 2 = 25%, 3 = 10%
    uint8_t idle_timeout_idx  = 0;     // 0 = OFF, 1 = 1m, 2 = 3m, 3 = 5m
    uint8_t default_page      = 0;     // PAGE_RACE = 0

    // Manual Simulation & Car Class Overrides (0 = Auto / SimHub)
    uint8_t game_override_idx  = 0;    // 0=AUTO, 1=AC, 2=iRacing, 3=AMS2, 4=LMU, 5=F1, 6=EA WRC
    uint8_t class_override_idx = 0;    // 0=AUTO, 1=GT3, 2=GT4, 3=F1, 4=HYPERCAR, 5=CUP, 6=TCR, 7=ROAD
};

extern const char* game_override_names[];
extern const size_t NUM_GAME_OVERRIDES;
extern const char* class_override_names[];
extern const size_t NUM_CLASS_OVERRIDES;

extern AppConfig app_config;

// =============================================================================
// CONFIGURATION STATUS ENUM
// =============================================================================
enum ConfigStatus {
    CONFIG_LOADED_FROM_SD,
    CONFIG_LOADED_FROM_NVS,
    CONFIG_CREATED_DEFAULT,
    CONFIG_ERROR
};

// Function prototypes implemented in config_manager.cpp
bool save_config_to_nvs();
ConfigStatus load_or_create_config(const char* filepath = "/beersoft_dash.cfg");