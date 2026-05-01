#include <Arduino.h>
#include <stdint.h>

#include "motors.h"

void performSetup()
{
    motors_init();
}

int onConnectionRequested()
{
    return 0;
}

int prepareTxPacket(uint8_t *data, uint8_t capacity, uint8_t &len)
{
    len = 0;
    return 0;
}

int onReceive(const uint8_t *data, uint8_t len)
{
    int8_t left = reinterpret_cast<const int8_t*>(data)[0];
    int8_t right = reinterpret_cast<const int8_t*>(data)[1];
    Serial.printf("Motor values: (%d, %d)\n", left, right);
    motor_left(left);
    motor_right(right);
    return 0;
}
