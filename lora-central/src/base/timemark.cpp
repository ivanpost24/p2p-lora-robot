#include "base/timemark.hpp"

#include <Arduino.h>

namespace {

    volatile unsigned long _mark = 0;

}

void timemark::mark()
{
    _mark = micros();
}

unsigned long timemark::timeSinceMark()
{
    return micros() - _mark;
}
