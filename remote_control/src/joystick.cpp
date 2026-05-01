#include "joystick.h"
#include "display.h"
#include "motors.h"
#include "config.h"

// ============================================================
// joystick.cpp — KY-023 reading with deadzone + axis scaling
//
// Joystick is powered from the Heltec 3.3 V pin, so VRx/VRy
// swing 0–3.3 V, which maps cleanly into the 12-bit ADC range
// (0–4095) without any voltage divider needed.
// ============================================================

static int8_t _read_axis(uint8_t pin) {
    int raw      = analogRead(pin);       // 0–4095
    int centered = raw - ADC_CENTER;      // ~-2048 … +2047

    if (centered > -ADC_DEADZONE && centered < ADC_DEADZONE) {
        return 0;
    }

    // Divide by 4 → ±512 max.
    // After tank mixing (L = Y+X, R = Y-X) worst case is 1024,
    // which constrain() clips to ±255 before it hits the motors.
    return centered / 17;
}

void joystick_init() {
    pinMode(PIN_SW, INPUT_PULLUP);
    // ADC pins need no pinMode on ESP32
}

JoyState joystick_read() {
    JoyState s;
    s.x      = -_read_axis(PIN_VRY);
    s.y      = -_read_axis(PIN_VRX);
    return s;
}



// ============================================================
// joystick_debug_print
//
// Shows both the raw 12-bit ADC value and the processed output
// side by side so you can verify:
//   1. ADC is reading — raw should sweep 0–4095 as you move stick
//   2. Center sits near 2048 on both axes when untouched
//   3. Deadzone works — processed stays 0 near center
//   4. Scaling is correct — processed maxes around ±512 at extremes
//   5. Button registers PRESS / ---- on click
//
// Requires display_init() to have run before this is called.
// ============================================================
void joystick_debug_print() {
    int raw_x  = analogRead(PIN_VRX);
    int raw_y  = analogRead(PIN_VRY);
    bool btn   = (digitalRead(PIN_SW) == LOW);
    int proc_x = -_read_axis(PIN_VRY);
    int proc_y = -_read_axis(PIN_VRX);

    JoyState s = joystick_read();

    // --- Serial --------------------------------------------------
    // %-5d: left-justify in a 5-char field so columns stay aligned
    // even when values go negative. Paste into the Arduino Serial
    // Plotter (Tools > Serial Plotter in VSCode PIO) for live graphs.
    Serial.printf("RAW  X:%-5d Y:%-5d  |  PROC X:%-5d Y:%-5d  |  BTN:%s\n",
                  raw_x, raw_y, proc_x, proc_y, btn ? "PRESS" : "----");

    // --- OLED ----------------------------------------------------
    // 8 rows available (64px tall / 8px font height).
    display_clear();
    display_print_line(0, "Joystick Test", 0);
    display_print_line(2, "Raw  X: ", raw_x);
    display_print_line(3, "Raw  Y: ", raw_y);
    display_print_line(5, "Proc X: ", s.x);
    display_print_line(6, "Proc Y: ", s.y);
    display_print_line(7, "Btn:    ", btn ? 1 : 0);
    display_show();
}