#include <Arduino.h>
#include <stdint.h>

#include "joystick.h"
#include "display.h"

static constexpr float fullForwardMagnitude = 0.6f;

void performSetup()
{
    joystick_init();
    display_init();
}

int onPeripheralDetected()
{
    return 0;
}

int prepareTxPacket(uint8_t *data, uint8_t capacity, uint8_t &len)
{
    JoyState joysticks = joystick_read();
    float linear = fullForwardMagnitude * joysticks.y;
    float angular = -(1 - fullForwardMagnitude) * joysticks.x;
    int8_t left = static_cast<int8_t>(linear - angular);
    int8_t right = static_cast<int8_t>(linear + angular);
    data[0] = *reinterpret_cast<uint8_t*>(&left);
    data[1] = *reinterpret_cast<uint8_t*>(&right);
    len = 2;
    return 0;
}

int onReceive(const uint8_t *data, uint8_t len)
{
    return 0;
}
