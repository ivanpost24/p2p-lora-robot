
// Use for testing joy stick

#include <Arduino.h>
//#include "config.h"
//#include "display.h"
//#include "joystick.h"

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Beginning display init");
    // display_init();
    // Serial.println("display done");
    // joystick_init();
    // Serial.println("Joystick init done");
    // Serial.println("Joystick test ready. Move the stick and watch.");
}

void loop() {
    //joystick_debug_print();
    Serial.println("hello");
    delay(100);
}