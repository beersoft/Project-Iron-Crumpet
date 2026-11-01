#pragma once

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "LovyanGFX_Driver.h"
#include "ui_settings.h"

extern LGFX gfx;

struct DashElementDef;

// Color and Math Utilities
uint16_t dim_color565(uint16_t color, float scale = 0.22f);

// Low-Level Graphic Primitives
void draw_element_skeleton(const DashElementDef& el);
void draw_data_value_autofit(int x, int y, int w, int h, const char* val_str, uint16_t color, bool has_label);
void draw_bar_element(const DashElementDef& el, float val, bool full_draw);
void draw_dial_element(const DashElementDef& el, float val, bool full_draw);
void draw_radar_element(const DashElementDef& el, bool full_draw);