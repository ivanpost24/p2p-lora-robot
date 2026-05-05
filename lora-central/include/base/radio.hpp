#ifndef WIOT_RADIO_HPP
#define WIOT_RADIO_HPP

#include <functional>
#include <RadioLib.h>
#include "loraconn.hpp"

namespace radio {

int16_t setup(
    uint8_t spreadingFactor,
    int8_t txPower = 20,
    uint8_t codingRate = 5,
    uint8_t syncWord = 0x34,
    uint16_t preamble = 8
);

uint8_t getChannel();

int16_t setChannel(uint8_t channel);

int16_t startTransmitting(const loraconn::Packet& packet);

RadioLibTime_t getApproximateTimeOnAir(uint8_t payloadLength);

int16_t startReceiving();

int16_t finishReceiving();

bool readPacket(loraconn::Packet& out);

bool readPacket(std::function<void(const loraconn::Packet&)> responder);

bool readAdvertisement(std::function<void(const loraconn::Advertisement&)> responder);

bool readConnectionRequest(std::function<void(const loraconn::ConnectionRequest&)> responder);

bool readConnectionData(
    std::function<void(const loraconn::ConnectionData&)> responder,
    std::function<void(const loraconn::DisconnectionRequest&)> disconnRequestResponder = nullptr
);

bool currentlyReceivingPacket();

bool pollCompletedOperation();

}

#endif