#pragma once

// ============================================================
// config.h — pin assignments and tuning constants
//
// SCHEMATIC NOTES (training-robot-schem):
//   U1 = RIGHT side motors (FR+, FR-, BR+, BR-)
//   U2 = LEFT  side motors (FL+, FL-, BL+, BL-)
//
//   STBY is driven by physical switch SW1 (10K pulldown + LED
//   indicator). J1-4 also connects to this net, so GPIO can
//   pull STBY LOW for e-stop — but SW1 must be ON for GPIO
//   to be able to bring STBY HIGH. Both must agree.
//
// PIN VERIFICATION CHECKLIST:
//   Before first motor test, verify with a multimeter that
//   each Heltec GPIO actually reaches the correct TB6612 pin.
//   The assignments below are from the hardware brief and may
//   need adjustment based on your actual jumper wiring.
// ============================================================

// ------------------------------------------------------------
// TB6612FNG — U1 (RIGHT side: FR, BR motors)
// Schematic: U1 AO1=FR+  AO2=FR-  BO1=BR+  BO2=BR-
// ------------------------------------------------------------
#define PIN_PWMR   19   // U1 PWMA  — right speed
#define PIN_RIN1   33   // U1 AIN1  — right forward
#define PIN_RIN2   20   // U1 AIN2  — right backward
                        // ⚠ GPIO21 also = OLED RST on Heltec V3 PCB
                        //   Fine for motor-only test, remap (35/36)
                        //   before combining with OLED code

// ------------------------------------------------------------
// TB6612FNG — U2 (LEFT side: FL, BL motors)
// Schematic: U2 AO1=FL+  AO2=FL-  BO1=BL+  BO2=BL-
// ------------------------------------------------------------
#define PIN_PWML   26   // U2 PWMA  — left speed  (PWMA on U2, not PWMB)
#define PIN_LIN1   48   // U2 AIN1  — left forward
#define PIN_LIN2   47   // U2 AIN2  — left backward

// ------------------------------------------------------------
// STBY — shared net, both chips
// Schematic: SW1 physical switch also on this net.
//   SW1 must be in ON position for GPIO to assert HIGH.
//   GPIO pulling LOW always wins (e-stop still works).
// ------------------------------------------------------------

// ------------------------------------------------------------
// KY-023 Joystick — power from 3.3V pin (NOT VUSB/5V)
// ADC max is 3.3V; powering from 5V would damage ADC inputs.
// ------------------------------------------------------------
#define PIN_VRX    7    // ADC1_CH6 — safe analog input
#define PIN_VRY    6    // ADC1_CH5 — safe analog input
#define PIN_SW     5   // 

// ------------------------------------------------------------
// HC-SR04 Ultrasonic — stubbed, implement later
// ------------------------------------------------------------
#define PIN_TRIG   10
#define PIN_ECHO   11
#define SONAR_MAX_CM 200

// ------------------------------------------------------------
// LEDC (PWM) — ESP32-S3 hardware PWM peripheral
// TB6612 max PWM freq: 100 kHz. 1 kHz is safe and inaudible.
// 8-bit resolution → duty 0–255.
// ------------------------------------------------------------
#define LEDC_CH_LEFT    0
#define LEDC_CH_RIGHT   1
#define LEDC_FREQ_HZ    1000
#define LEDC_RES_BITS   8

// ------------------------------------------------------------
// Joystick ADC tuning
// ADC_CENTER: 12-bit midpoint. KY-023 at 3.3V → center ≈ 2048.
// ADC_DEADZONE: raw counts either side of center treated as zero.
//   80 counts ≈ 64 mV — covers pot noise and mechanical slop.
// ------------------------------------------------------------
#define ADC_CENTER    2024 // 1024
#define ADC_DEADZONE    120 //  80

// ------------------------------------------------------------
// Motor test sequence (used in startup verification)
// ------------------------------------------------------------
#define TEST_DUTY       150   // PWM duty for startup test (0-255)
#define TEST_DURATION_MS 400  // how long each test pulse runs

// ------------------------------------------------------------
// Control loop timing
// ------------------------------------------------------------
#define LOOP_PERIOD_MS   20   // 50 Hz
