#include <Arduino.h>
#include <stdint.h>

#include "base/joystick.h"
#include "base/display.h"
#include "base/controller.hpp"

constexpr uint8_t controller::centralPayloadLength = 2;

namespace {
    constexpr float fullForwardMagnitude = 0.6f;
}

void controller::setup()
{
    joystick_init();
    display_init();
}

int controller::onPeripheralDetected()
{
    return 0;
}

int controller::prepareTxPacket(uint8_t *data)
{
    JoyState joysticks = joystick_read();
    float linear = fullForwardMagnitude * joysticks.y;
    float angular = -(1 - fullForwardMagnitude) * joysticks.x;
    int8_t left = static_cast<int8_t>(linear - angular);
    int8_t right = static_cast<int8_t>(linear + angular);
    data[0] = *reinterpret_cast<uint8_t*>(&left);
    data[1] = *reinterpret_cast<uint8_t*>(&right);
    return 0;
}

int controller::onReceive(const uint8_t *data, uint8_t len)
{
    return 0;
}
