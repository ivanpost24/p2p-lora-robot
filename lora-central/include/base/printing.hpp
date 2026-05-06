#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace printing {

void error(const char *message, int16_t state);

void error(const __FlashStringHelper *message, int16_t state);

}
