#pragma once
#include <Arduino.h>

// ============================================================
// display.h — Heltec V3 onboard SSD1306 OLED (128x64)
//
// Internal wiring on the Heltec V3 PCB:
//   SDA → GPIO 17
//   SCL → GPIO 18
//   RST → GPIO 21   ← conflicts with PIN_AIN2 in config.h!
//                      Remap AIN2 before merging into full
//                      robot code (GPIO 35 or 36 are free).
//
// This wrapper keeps Adafruit library calls out of main.cpp
// so you can swap display libraries later without touching
// anything else.
// ============================================================

void display_init();

// Clear the screen and reset the cursor to top-left
void display_clear();

// Print a line of text at a given row (row 0–7 for 8px font)
void display_print_line(uint8_t row, const char* label, int value);

// Push the frame buffer to the physical screen.
// Call once per loop after all display_print_line() calls.
void display_show();