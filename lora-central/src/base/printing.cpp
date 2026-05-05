#include "base/printing.hpp"

#include <Arduino.h>
#include <stdint.h>

void printing::error(const char *message, int16_t state)
{
    Serial.printf("ERROR: %s (error code %d).\n", message, state);
}

void printing::error(const __FlashStringHelper *message, int16_t state)
{
    Serial.print(F("ERROR: "));
    Serial.print(message);
    Serial.print(F(" (error code "));
    Serial.print(state);
    Serial.println(").");
}
