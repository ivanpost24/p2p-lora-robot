#include "display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// display.cpp — SSD1306 128x64 OLED via I2C
// ============================================================

#define OLED_WIDTH  128
#define OLED_HEIGHT  64
#define OLED_ADDR   0x3C   // default I2C address for SSD1306 modules
#define OLED_SDA      17
#define OLED_SCL      18
#define OLED_RST      21   // ⚠️ conflicts with PIN_AIN2 — see display.h note

static Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RST);

void display_init() {
    // Start I2C on the Heltec's internal OLED pins
    Wire.begin(OLED_SDA, OLED_SCL);

    if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) { // pulled from datasheet of OLED
        Serial.println("[OLED] Init failed — check wiring or I2C address");
        return;
    }

    oled.clearDisplay();
    oled.setTextSize(1);       // 6x8 px font — fits 21 chars per row, 8 rows
    oled.setTextColor(SSD1306_WHITE);
    oled.display();
}

void display_clear() {
    oled.clearDisplay();
    oled.setCursor(0, 0);
}

// Prints "label: value" at a pixel y-position derived from row number.
// Row 0 = top, each row is 8 pixels tall → 8 rows on a 64px display.
void display_print_line(uint8_t row, const char* label, int value) {
    oled.setCursor(0, row * 8);
    oled.print(label);
    oled.print(value);
}

void display_show() {
    oled.display();   // pushes the internal frame buffer to the screen
}