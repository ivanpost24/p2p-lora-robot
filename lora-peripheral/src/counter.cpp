#include <Arduino.h>
#include <stdint.h>

static int counter = 0;

void performSetup()
{

}

int onConnectionRequested()
{
    counter = 0;
    return 0;
}

int prepareTxPacket(uint8_t *data, uint8_t capacity, uint8_t &len)
{
    if (capacity < 4) {
        return -1;
    }
    counter += 1;
    if (counter > 100) {
        return 1;
    }
    data[0] = counter >> 24;
    data[1] = (counter >> 16) & 0xff;
    data[2] = (counter >> 8) & 0xff;
    data[3] = counter & 0xff;
    len = 4;
    return 0;
}

int onReceive(const uint8_t *data, uint8_t len)
{
    counter = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    Serial.print("Received value: ");
    Serial.println(counter);
    return 0;
}
