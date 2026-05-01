#pragma once
#include <Arduino.h>

// ============================================================
// joystick.h — KY-023 analog joystick
// ============================================================

struct JoyState {
    int  x;            // -512 … +512, deadzone zeroed
    int  y;            // -512 … +512, deadzone zeroed
    bool button;       // true = pressed
};

void      joystick_init();
JoyState  joystick_read();

// Prints raw ADC values, deadzone-processed values, and button state
// to both Serial and the OLED display. Call this instead of the normal
// read during testing. Requires display_init() to have run first.
void      joystick_debug_print();