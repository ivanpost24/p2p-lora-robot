#ifndef WIOT_PRINTING_HPP
#define WIOT_PRINTING_HPP

#include <Arduino.h>
#include <stdint.h>

namespace printing {

void error(const char *message, int16_t state);

void error(const __FlashStringHelper *message, int16_t state);

}

#endif