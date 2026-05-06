#include "controller.hpp"

#include <Arduino.h>
#include <stdint.h>

#include "motors.h"

constexpr uint8_t controller::peripheralPayloadLength = 0;

void controller::setup()
{
    motors_init();
}

int controller::onConnectionRequested()
{
    return 0;
}

int controller::prepareTxPacket(uint8_t *data)
{
    return 0;
}

int controller::onReceive(const uint8_t *data, uint8_t len)
{
    int8_t left = reinterpret_cast<const int8_t*>(data)[0];
    int8_t right = reinterpret_cast<const int8_t*>(data)[1];
    Serial.printf("Motor values: (%d, %d)\n", left, right);
    motor_left(left);
    motor_right(right);
    return 0;
}
