// =============================================================================
// Build Settings Validation
// =============================================================================
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
  #error "Wrong Target! Please select 'ESP32S3 Dev Module' in Tools -> Board."
#endif

#if !defined(BOARD_HAS_PSRAM)
  #error "PSRAM is disabled! LovyanGFX RGB 800x480 requires PSRAM. Set Tools -> PSRAM -> 'OPI PSRAM'."
#endif

#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT == 0
  #warning "USB CDC On Boot is disabled. Serial monitor over native USB may not work."
#endif

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
// TARGET BOARD SELECTION (Uncomment ONE):
// =============================================================================
//#define BOARD_ELECROW_DIS02170A          // Elecrow Advance 7.0" (SC7277 / GT911 v1.4)
 #define BOARD_ELECROW_DIS07050H       // Elecrow Basic 5.0" (ILI6122, direct GPIO 2 BL)
// #define BOARD_SUNTON_ESP32_8048S043   // Sunton 4.3" 800x480 (ST7701S / GT911)
// =============================================================================

// -----------------------------------------------------------------------------
// Pin Mappings & Expander for Elecrow CrowPanel Advance 7.0" (DIS02170A)
// -----------------------------------------------------------------------------
#if defined(BOARD_ELECROW_DIS02170A)
  #define SD_MOSI 6
  #define SD_MISO 4
  #define SD_SCK  5
  #define SD_CS   0

  #define EXPANDER_I2C_ADDR 0x18

  inline void init_crowpanel_advance_power() {
      // Advance 7.0" routes I2C to GPIO 15 (SDA) and GPIO 16 (SCL)
      Wire.begin(15, 16);
      delay(20);

      // Set all IOs to outputs (Register 0x03)
      Wire.beginTransmission(EXPANDER_I2C_ADDR);
      Wire.write(0x03); 
      Wire.write(0x00);
      Wire.endTransmission();
      delay(20);

      // Pull outputs high to assert LCD power & release resets
      Wire.beginTransmission(EXPANDER_I2C_ADDR);
      Wire.write(0x01);
      Wire.write(0xFF);
      Wire.endTransmission();
      delay(50);
  }

#elif defined(BOARD_ELECROW_DIS07050H)
  #define SD_MOSI 11
  #define SD_MISO 13
  #define SD_SCK  12
  #define SD_CS   10

  #define PCA9557_ADDR 0x18

  inline void init_crowpanel_dis07050h_power() {
      Wire.begin(19, 20);
      delay(20);

      Wire.beginTransmission(PCA9557_ADDR);
      Wire.write(0x03); 
      Wire.write(0x00);
      Wire.endTransmission();

      Wire.beginTransmission(PCA9557_ADDR);
      Wire.write(0x01); 
      Wire.write(0x00);
      Wire.endTransmission();
      delay(20);

      Wire.beginTransmission(PCA9557_ADDR);
      Wire.write(0x01); 
      Wire.write(0x01);
      Wire.endTransmission();
      delay(100);

      Wire.beginTransmission(PCA9557_ADDR);
      Wire.write(0x03);
      Wire.write(0x02);
      Wire.endTransmission();
      delay(20);
  }

#else // BOARD_SUNTON_ESP32_8048S043
  #define SD_MOSI 11
  #define SD_MISO 13
  #define SD_SCK  12
  #define SD_CS   10
#endif

// -----------------------------------------------------------------------------
// SD Card Global Bus & Init Helper
// -----------------------------------------------------------------------------
extern SPIClass sd_spi;

inline bool init_board_sd_card(uint32_t freq = 4000000) {
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    delay(50);

    sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    delay(50);

    if (SD.begin(SD_CS, sd_spi, freq)) {
        return true;
    }

    SD.end();
    delay(30);
    return SD.begin(SD_CS, sd_spi, 400000);
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

#if defined(BOARD_ELECROW_DIS02170A)
      // -----------------------------------------------------------------------
      // Elecrow CrowPanel Advance 7.0" (DIS02170A v1.4) Pinout
      // -----------------------------------------------------------------------
      // Blue Channel (B0 - B4)
      cfg.pin_d0  = GPIO_NUM_21;
      cfg.pin_d1  = GPIO_NUM_47;
      cfg.pin_d2  = GPIO_NUM_48;
      cfg.pin_d3  = GPIO_NUM_45;
      cfg.pin_d4  = GPIO_NUM_38;

      // Green Channel (G0 - G5)
      cfg.pin_d5  = GPIO_NUM_9;
      cfg.pin_d6  = GPIO_NUM_10;
      cfg.pin_d7  = GPIO_NUM_11;
      cfg.pin_d8  = GPIO_NUM_12;
      cfg.pin_d9  = GPIO_NUM_13;
      cfg.pin_d10 = GPIO_NUM_14;

      // Red Channel (R0 - R4)
      cfg.pin_d11 = GPIO_NUM_7;
      cfg.pin_d12 = GPIO_NUM_17;
      cfg.pin_d13 = GPIO_NUM_18;
      cfg.pin_d14 = GPIO_NUM_3;
      cfg.pin_d15 = GPIO_NUM_46;

      // Sync & Control Pins
      cfg.pin_henable = GPIO_NUM_42;
      cfg.pin_vsync   = GPIO_NUM_41;
      cfg.pin_hsync   = GPIO_NUM_40;
      cfg.pin_pclk    = GPIO_NUM_39;

      cfg.freq_write        = 16000000;
      cfg.hsync_polarity    = 0;
      cfg.hsync_front_porch = 8;
      cfg.hsync_pulse_width = 4;
      cfg.hsync_back_porch  = 8;
      cfg.vsync_polarity    = 0;
      cfg.vsync_front_porch = 8;
      cfg.vsync_pulse_width = 4;
      cfg.vsync_back_porch  = 8;
      cfg.pclk_active_neg   = 1;
      cfg.pclk_idle_high    = 0;
      cfg.de_idle_high      = 0;

#elif defined(BOARD_SUNTON_ESP32_8048S043)
      cfg.pin_d0  = GPIO_NUM_8;  cfg.pin_d1  = GPIO_NUM_3;  cfg.pin_d2  = GPIO_NUM_46; cfg.pin_d3  = GPIO_NUM_9;  cfg.pin_d4  = GPIO_NUM_1;
      cfg.pin_d5  = GPIO_NUM_5;  cfg.pin_d6  = GPIO_NUM_6;  cfg.pin_d7  = GPIO_NUM_7;  cfg.pin_d8  = GPIO_NUM_15; cfg.pin_d9  = GPIO_NUM_16; cfg.pin_d10 = GPIO_NUM_4;
      cfg.pin_d11 = GPIO_NUM_45; cfg.pin_d12 = GPIO_NUM_48; cfg.pin_d13 = GPIO_NUM_47; cfg.pin_d14 = GPIO_NUM_21; cfg.pin_d15 = GPIO_NUM_14;

      cfg.pin_henable = GPIO_NUM_40;
      cfg.pin_vsync   = GPIO_NUM_41;
      cfg.pin_hsync   = GPIO_NUM_39;
      cfg.pin_pclk    = GPIO_NUM_42;
      cfg.freq_write        = 16000000;
      cfg.hsync_polarity    = 0; 
      cfg.hsync_front_porch = 8; 
      cfg.hsync_pulse_width = 4; 
      cfg.hsync_back_porch  = 16;
      cfg.vsync_polarity    = 0; 
      cfg.vsync_front_porch = 4; 
      cfg.vsync_pulse_width = 4; 
      cfg.vsync_back_porch  = 4;
      cfg.pclk_active_neg   = 1; 
      cfg.pclk_idle_high    = 0; 
      cfg.de_idle_high      = 0;

#else // Elecrow Basic DIS07050H
      cfg.pin_d0  = GPIO_NUM_8;  cfg.pin_d1  = GPIO_NUM_3;  cfg.pin_d2  = GPIO_NUM_46; cfg.pin_d3  = GPIO_NUM_9;  cfg.pin_d4  = GPIO_NUM_1;
      cfg.pin_d5  = GPIO_NUM_5;  cfg.pin_d6  = GPIO_NUM_6;  cfg.pin_d7  = GPIO_NUM_7;  cfg.pin_d8  = GPIO_NUM_15; cfg.pin_d9  = GPIO_NUM_16; cfg.pin_d10 = GPIO_NUM_4;
      cfg.pin_d11 = GPIO_NUM_45; cfg.pin_d12 = GPIO_NUM_48; cfg.pin_d13 = GPIO_NUM_47; cfg.pin_d14 = GPIO_NUM_21; cfg.pin_d15 = GPIO_NUM_14;

      cfg.pin_henable = GPIO_NUM_40;
      cfg.pin_vsync   = GPIO_NUM_41;
      cfg.pin_hsync   = GPIO_NUM_39;
      cfg.pin_pclk    = GPIO_NUM_0;
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

    // 3. Backlight Control
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
      cfg.freq            = 400000;
      cfg.pin_int         = -1;
      cfg.pin_rst         = -1;
      cfg.i2c_addr        = 0x5D;

#if defined(BOARD_ELECROW_DIS02170A)
      cfg.i2c_port        = I2C_NUM_0;
      cfg.pin_sda         = GPIO_NUM_15; // Advance 7.0" SDA
      cfg.pin_scl         = GPIO_NUM_16; // Advance 7.0" SCL
      cfg.x_min           = 0;
      cfg.x_max           = 799;
      cfg.y_min           = 0;
      cfg.y_max           = 479;
#elif defined(BOARD_SUNTON_ESP32_8048S043)
      cfg.i2c_port        = I2C_NUM_0;
      cfg.pin_sda         = GPIO_NUM_19;
      cfg.pin_scl         = GPIO_NUM_20;
      cfg.x_min           = 0;
      cfg.x_max           = 479;
      cfg.y_min           = 0;
      cfg.y_max           = 271;
#else
      cfg.i2c_port        = I2C_NUM_0;
      cfg.pin_sda         = GPIO_NUM_19;
      cfg.pin_scl         = GPIO_NUM_20;
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