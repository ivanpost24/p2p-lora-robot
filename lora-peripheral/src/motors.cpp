#include "motors.h"
#include "config.h"

// ============================================================
// motors.cpp — TB6612FNG + LEDC implementation
// ============================================================

// ------------------------------------------------------------
// Internal helper — set one TB6612 channel
//
// TB6612 truth table (active drive mode):
//   IN1  IN2   Motor output
//    H    L    Forward
//    L    H    Reverse
//    L    L    Coast (both outputs hi-Z)
//    H    H    Brake (both outputs pulled to GND) — avoid,
//              causes shoot-through on the H-bridge
// ------------------------------------------------------------
static void _set_channel(int8_t speed, uint8_t ledc_ch,
                          uint8_t pin_fwd, uint8_t pin_rev) {
    if (speed == 0) {
        digitalWrite(pin_fwd, LOW);
        digitalWrite(pin_rev, LOW);
        ledcWrite(ledc_ch, 0);
        return;
    }

    if (speed > 0) {
        digitalWrite(pin_fwd, HIGH);
        digitalWrite(pin_rev, LOW);
    } else {
        digitalWrite(pin_fwd, LOW);
        digitalWrite(pin_rev, HIGH);
        speed = -speed;
    }

    ledcWrite(ledc_ch, (uint8_t)speed);
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------

void motors_init() {
    pinMode(PIN_RIN1, OUTPUT);
    pinMode(PIN_RIN2, OUTPUT);
    pinMode(PIN_LIN1, OUTPUT);
    pinMode(PIN_LIN2, OUTPUT);

    ledcSetup(LEDC_CH_LEFT,  LEDC_FREQ_HZ, LEDC_RES_BITS);
    ledcSetup(LEDC_CH_RIGHT, LEDC_FREQ_HZ, LEDC_RES_BITS);
    ledcAttachPin(PIN_PWMR, LEDC_CH_LEFT);
    ledcAttachPin(PIN_PWML, LEDC_CH_RIGHT);

    motors_stop();
}

void motor_left(int8_t speed) {
    _set_channel(speed, LEDC_CH_LEFT, PIN_LIN1, PIN_LIN2);
}

void motor_right(int8_t speed) {
    _set_channel(speed, LEDC_CH_RIGHT, PIN_RIN1, PIN_RIN2);
}

void motors_stop() {
    motor_left(0);
    motor_right(0);
}
