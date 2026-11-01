// logos.h - Rev 2.2.0
// Maps arrays for TC, ABS, and Engine Maps using icons.h
#pragma once

#include <Arduino.h>
#include "icons.h" // Pulls directly from your icons.h file

// ==========================================
// ASSIST & ELECTRONICS ARRAYS (0 to 12)
// ==========================================

const unsigned char* tc_array[13] = {
    epd_bitmap_Tracion_Control,
    epd_bitmap_Tracion_Control_1,
    epd_bitmap_Tracion_Control_2,
    epd_bitmap_Tracion_Control_3,
    epd_bitmap_Tracion_Control_4,
    epd_bitmap_Tracion_Control_5,
    epd_bitmap_Tracion_Control_6,
    epd_bitmap_Tracion_Control_7,
    epd_bitmap_Tracion_Control_8,
    epd_bitmap_Tracion_Control_9,
    epd_bitmap_Tracion_Control_10,
    epd_bitmap_Tracion_Control_11,
    epd_bitmap_Tracion_Control_12,
};

const unsigned char* abs_array[13] = {
    epd_bitmap_ABS,
    epd_bitmap_ABS_1,
    epd_bitmap_ABS_2,
    epd_bitmap_ABS_3,
    epd_bitmap_ABS_4,
    epd_bitmap_ABS_5,
    epd_bitmap_ABS_6,
    epd_bitmap_ABS_7,
    epd_bitmap_ABS_8,
    epd_bitmap_ABS_9,
    epd_bitmap_ABS_10,
    epd_bitmap_ABS_11,
    epd_bitmap_ABS_12
};

const unsigned char* Engine_Mode__array[13] = {
    epd_bitmap_Engine_Mode_Presets,
    epd_bitmap_Engine_Mode_1,
    epd_bitmap_Engine_Mode_2,
    epd_bitmap_Engine_Mode_3,
    epd_bitmap_Engine_Mode_4,
    epd_bitmap_Engine_Mode_5,
    epd_bitmap_Engine_Mode_6,
    epd_bitmap_Engine_Mode_7,
    epd_bitmap_Engine_Mode_8,
    epd_bitmap_Engine_Mode_9,
    epd_bitmap_Engine_Mode_10,
    epd_bitmap_Engine_Mode_11,
    epd_bitmap_Engine_Mode_12
};
