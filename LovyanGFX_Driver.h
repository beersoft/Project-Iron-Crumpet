#pragma once
#pragma once

#define LCD_H_RES 800
#define LCD_V_RES 480

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

// =============================================================================
// SELECT ACTIVE TARGET BOARD (Uncomment ONE):
// =============================================================================
// #define BOARD_ELECROW_DIS02050A       // Elecrow Advance 5.0" (ST7262 + STC8H)
 #define BOARD_ELECROW_DIS07050H       // Elecrow Basic 5.0" (ILI6122, direct GPIO 2 BL)
// #define BOARD_SUNTON_ESP32_8048S043   // Sunton 4.3" 800x480 (ST7701S / GT911)
// =============================================================================

// -----------------------------------------------------------------------------
// Hardware Power & Expander Helpers (Elecrow Basic)
// -----------------------------------------------------------------------------
#define PCA9557_ADDR 0x18  // Default I2C address for PCA9557 on CrowPanel[cite: 1]

inline void init_crowpanel_dis07050h_power() {
    Wire.begin(19, 20); // SDA: GPIO 19, SCL: GPIO 20[cite: 1]
    delay(20);

    // 1. Reset / Set Output Mode on PCA9557 (Register 0x03 = Configuration Register)[cite: 1]
    Wire.beginTransmission(PCA9557_ADDR);
    Wire.write(0x03); 
    Wire.write(0x00); // 0 = Output[cite: 1]
    Wire.endTransmission();

    // 2. Drive IO0 and IO1 LOW[cite: 1]
    Wire.beginTransmission(PCA9557_ADDR);
    Wire.write(0x01); 
    Wire.write(0x00);
    Wire.endTransmission();
    delay(20);

    // 3. Pull IO0 HIGH (Enables Backlight & Power rail)[cite: 1]
    Wire.beginTransmission(PCA9557_ADDR);
    Wire.write(0x01); 
    Wire.write(0x01); // IO0 HIGH, IO1 LOW[cite: 1]
    delay(100);

    // 4. Set IO1 back to input/high-Z for touch/reset release[cite: 1]
    Wire.beginTransmission(PCA9557_ADDR);
    Wire.write(0x03);
    Wire.write(0x02); // IO1 as INPUT (bit 1 = 1)[cite: 1]
    Wire.endTransmission();
    delay(20);
}

// -----------------------------------------------------------------------------
// SD Card Pin Mapping per Board
// -----------------------------------------------------------------------------
#if defined(BOARD_ELECROW_DIS07050H)
  #define SD_MOSI 11
  #define SD_MISO 13
  #define SD_SCK  12
  #define SD_CS   10

#elif defined(BOARD_SUNTON_ESP32_8048S043)
  #define SD_MOSI 11
  #define SD_MISO 13
  #define SD_SCK  12
  #define SD_CS   10

#else // BOARD_ELECROW_DIS02050A (Advance)
  #define SD_MOSI 6
  #define SD_MISO 4
  #define SD_SCK  5
  #define SD_CS   0
#endif

// -----------------------------------------------------------------------------
// SD Card Global Bus & Init Helper
// -----------------------------------------------------------------------------
extern SPIClass sd_spi;

inline bool init_board_sd_card(uint32_t freq = 4000000) {
  sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  delay(30);
  return SD.begin(SD_CS, sd_spi, freq);
}

// -----------------------------------------------------------------------------
// LovyanGFX Device Class
// -----------------------------------------------------------------------------
class LGFX : public lgfx::LGFX_Device {
public:
  lgfx::Bus_RGB      _bus_instance;
  lgfx::Panel_RGB    _panel_instance;
  lgfx::Light_PWM    _light_instance;
  lgfx::Touch_GT911  _touch_instance;

  LGFX(void) {
    // 1. Panel Dimensions & Color Mapping
    {
      auto cfg = _panel_instance.config();
      cfg.memory_width  = 800;
      cfg.memory_height = 480;
      cfg.panel_width   = 800;
      cfg.panel_height  = 480;
      cfg.offset_x      = 0;
      cfg.offset_y      = 0;
      cfg.rgb_order     = false;
      _panel_instance.config(cfg);
    }

    {
      auto cfg = _panel_instance.config_detail();
      cfg.use_psram = 1;
      _panel_instance.config_detail(cfg);
    }

    // 2. RGB Bus Configuration & Data Lines
    {
      auto cfg = _bus_instance.config();
      cfg.panel = &_panel_instance;

      // Blue Channel (B0 - B4)
      cfg.pin_d0  = GPIO_NUM_8;
      cfg.pin_d1  = GPIO_NUM_3;
      cfg.pin_d2  = GPIO_NUM_46;
      cfg.pin_d3  = GPIO_NUM_9;
      cfg.pin_d4  = GPIO_NUM_1;

      // Green Channel (G0 - G5)
      cfg.pin_d5  = GPIO_NUM_5;
      cfg.pin_d6  = GPIO_NUM_6;
      cfg.pin_d7  = GPIO_NUM_7;
      cfg.pin_d8  = GPIO_NUM_15;
      cfg.pin_d9  = GPIO_NUM_16;
      cfg.pin_d10 = GPIO_NUM_4;

      // Red Channel (R0 - R4)
      cfg.pin_d11 = GPIO_NUM_45;
      cfg.pin_d12 = GPIO_NUM_48;
      cfg.pin_d13 = GPIO_NUM_47;
      cfg.pin_d14 = GPIO_NUM_21;
      cfg.pin_d15 = GPIO_NUM_14;

      // Sync & Control Pins
      cfg.pin_henable = GPIO_NUM_40;
      cfg.pin_vsync   = GPIO_NUM_41;
      cfg.pin_hsync   = GPIO_NUM_39;

#if defined(BOARD_SUNTON_ESP32_8048S043)
      // Confirmed 16MHz Sunton 4.3" Timings
cfg.pin_pclk          = GPIO_NUM_42; // Sunton dot clock[cite: 7]
      cfg.freq_write        = 16000000;    // 16 MHz required by ST7701S[cite: 2, 7]
      cfg.hsync_polarity    = 0;
      cfg.hsync_front_porch = 8;           //[cite: 2]
      cfg.hsync_pulse_width = 4;           //[cite: 2]
      cfg.hsync_back_porch  = 16;          //[cite: 2]
      cfg.vsync_polarity    = 0;
      cfg.vsync_front_porch = 4;           //[cite: 2]
      cfg.vsync_pulse_width = 4;           //[cite: 2]
      cfg.vsync_back_porch  = 4;           //[cite: 2]
      cfg.pclk_active_neg   = 1;           // 1 = Latch on falling edge[cite: 2, 7]
      cfg.pclk_idle_high    = 0;
      cfg.de_idle_high      = 0;

#else // Elecrow DIS02050A & DIS07050H
      cfg.pin_pclk          = GPIO_NUM_0;
      cfg.freq_write        = 12000000;
      cfg.hsync_polarity    = 0;
      cfg.hsync_front_porch = 16;
      cfg.hsync_pulse_width = 4;
      cfg.hsync_back_porch  = 48;
      cfg.vsync_polarity    = 0;
      cfg.vsync_front_porch = 10;
      cfg.vsync_pulse_width = 4;
      cfg.vsync_back_porch  = 12;
      cfg.pclk_active_neg   = 0;
      cfg.pclk_idle_high    = 0;
      cfg.de_idle_high      = 0;
#endif

      _bus_instance.config(cfg);
    }
    _panel_instance.setBus(&_bus_instance);

    // 3. Backlight Control (GPIO 2 PWM)
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl      = GPIO_NUM_2;
      cfg.freq        = 1000;
      cfg.pwm_channel = 7;
      cfg.invert      = false;
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    // 4. GT911 Capacitive Touch Controller
    {
      auto cfg = _touch_instance.config();
      cfg.bus_shared      = false;
      cfg.offset_rotation = 0;
      cfg.i2c_port        = I2C_NUM_0;       // I2C port 0 matching Wire default
      cfg.pin_sda         = GPIO_NUM_19;     // SDA
      cfg.pin_scl         = GPIO_NUM_20;     // SCL
      cfg.freq            = 400000;
      cfg.pin_int         = -1;              // Polling mode
      cfg.pin_rst         = -1;
      cfg.i2c_addr        = 0x5D;            // Verified GT911 address

#if defined(BOARD_SUNTON_ESP32_8048S043)
      // Mapped to factory 480x272 NVRAM digitizer firmware
      cfg.x_min           = 0;
      cfg.x_max           = 479;
      cfg.y_min           = 0;
      cfg.y_max           = 271;
#else
      cfg.x_min           = 0;
      cfg.x_max           = 799;
      cfg.y_min           = 0;
      cfg.y_max           = 479;
#endif

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};