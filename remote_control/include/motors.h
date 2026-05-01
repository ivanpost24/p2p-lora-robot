#pragma once
#include <Arduino.h>

// ============================================================
// motors.h — TB6612FNG abstraction
//
// Why not the SparkFun TB6612 library?
// That library calls analogWrite() internally, which isn't
// available on ESP32-S3. We wrap LEDC ourselves here —
// same clean API, works correctly on this chip.
// ============================================================

// Call once in setup()
void motors_init();

// Signed speed: -255 (full reverse) … 0 (coast) … +255 (full forward)
void motor_left(int speed);
void motor_right(int speed);

// Convenience wrappers
void motors_stop();        // coast both sides
