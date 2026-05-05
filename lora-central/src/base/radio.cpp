#include "base/radio.hpp"
#include "base/printing.hpp"

namespace {

    /**************** Pin assignment for the Heltec V3 board ******************/
    constexpr int LORA_CS    = 8;      // Chip select pin
    constexpr int LORA_MOSI  = 10;
    constexpr int LORA_MISO  = 11;
    constexpr int LORA_SCK   = 9;
    constexpr int LORA_NRST  = 12;      // Reset pin
    constexpr int LORA_DIO1  = 14;      // DIO1 switch
    constexpr int LORA_BUSY  = 13;
    constexpr int BUTTON     = 0;

    SX1262 _radio = new Module(LORA_CS, LORA_DIO1, LORA_NRST, LORA_BUSY);

    uint8_t buf[255];
    loraconn::Packet packet(255);

    volatile bool operationCompleted = false;
    volatile uint8_t _channel = loraconn::ADVERTISING_CHANNEL;

    #if defined(ESP8266) || defined(ESP32)
    ICACHE_RAM_ATTR
    #endif
    void onOperationCompleted(void)
    {
        operationCompleted = true;
    }

}

int16_t radio::setup(uint8_t spreadingFactor, int8_t txPower, uint8_t codingRate, uint8_t syncWord, uint16_t preamble)
{
    int16_t err;
    Serial.print(F("[SX1262] Initializing ... "));
    err = _radio.begin();
    if (err == RADIOLIB_ERR_NONE) {
        Serial.println(F("success"));
    } else {
        printing::error("Failed to start radio", err);
        return err;
    }

    err = _radio.setSpreadingFactor(spreadingFactor);
    if (err != RADIOLIB_ERR_NONE) {
        printing::error("Spreading factor initialization failed", err);
        return err;
    }
    Serial.print("[SX1262] Spreading factor:\t\t");
    Serial.println(spreadingFactor);

    err = _radio.setOutputPower(txPower);
    if (err != RADIOLIB_ERR_NONE) {
        printing::error("TX power initialization failed", err);
        return err;
    }
    Serial.print("[SX1262] Transmit power:\t\t");
    Serial.print(txPower);
    Serial.println(F(" dBm"));

    err = _radio.setCurrentLimit(140.0);
    if (err != RADIOLIB_ERR_NONE) {
        printing::error("Current limit intialization failed", err);
    }
    _radio.setDio1Action(onOperationCompleted);

    return RADIOLIB_ERR_NONE;
}

uint8_t radio::getChannel()
{
    return _channel;
}

int16_t radio::setChannel(uint8_t channel)
{
    int16_t err;
    err = _radio.setFrequency(loraconn::getChannelFrequency(channel));
    if (err != RADIOLIB_ERR_NONE) {
        Serial.print(F("Error tuning radio to channel "));
        Serial.print(channel);
        Serial.printf(" (error code %d)\n", err);
        return err;
    }
    _channel = channel;

    err = _radio.setBandwidth(loraconn::getChannelBandwidth(channel));
    if (err != RADIOLIB_ERR_NONE) {
        Serial.print(F("Error setting radio bandwidth (error code "));
        Serial.printf(" (error code %d)\n", err);
        return err;
    }

    Serial.print(F("Changed channel to "));
    Serial.println(channel);
    return RADIOLIB_ERR_NONE;
}

int16_t radio::startTransmitting(const loraconn::Packet &packet)
{
    operationCompleted = false;
    return _radio.startTransmit(packet.getRaw(), packet.getLength());
}

RadioLibTime_t radio::getApproximateTimeOnAir(uint8_t payloadLength)
{
    return _radio.getTimeOnAir(
        loraconn::ConnectionData::headerLength + loraconn::ConnectionData::dataHeaderLength + payloadLength
    );
}

int16_t radio::startReceiving()
{
    return _radio.startReceive();
}

int16_t radio::finishReceiving()
{
    int16_t err = _radio.finishReceive();
    operationCompleted = false;
    return err;
}

bool radio::readPacket(loraconn::Packet &out)
{
    size_t length = _radio.getPacketLength();
    if (length > 255) {
        printing::error("Read packet was too long", RADIOLIB_ERR_PACKET_TOO_LONG);
        return false;
    }
    int16_t err = _radio.readData(buf, length);
    if (err != RADIOLIB_ERR_NONE) {
        printing::error("Failed to receive packet", err);
        return false;
    } else if (!out.setRaw(buf, length)) {
        Serial.println(F("Received non-LoRaConn packet"));
        return false;
    } else {
        return true;
    }
}

bool radio::readPacket(std::function<void(const loraconn::Packet&)> responder)
{
    if (readPacket(packet)) {
        responder(packet);
        return true;
    } else {
        printing::error("Ignoring non-LoRaConn packet", 1);
        return false;
    }
}

bool radio::readAdvertisement(std::function<void(const loraconn::Advertisement&)> responder)
{
    if (readPacket(packet)) {
        if (packet.getPacketType() == loraconn::PacketType::ADVERT) {
            loraconn::Advertisement advert(packet);
            responder(advert);
            return true;
        } else {
            printing::error("Ignoring packet which is not an advertisement", 1);
            return false;
        }
    } else {
        printing::error("Ignoring non-LoRaConn packet", 1);
        return false;
    }
}

bool radio::readConnectionRequest(std::function<void(const loraconn::ConnectionRequest&)> responder)
{
    if (readPacket(packet)) {
        if (packet.getPacketType() == loraconn::PacketType::CONN_REQ) {
            loraconn::ConnectionRequest connRequest(packet);
            responder(connRequest);
            return true;
        } else {
            printing::error("Ignoring packet which is not a connection request", 1);
            return false;
        }
    } else {
        printing::error("Ignoring non-LoRaConn packet", 1);
        return false;
    }
}

bool radio::readConnectionData(
    std::function<void(const loraconn::ConnectionData &)> responder,
    std::function<void(const loraconn::DisconnectionRequest &)> disconnRequestResponder
)
{
    if (readPacket(packet)) {
        if (packet.getPacketType() == loraconn::PacketType::CONN_DATA) {
            loraconn::ConnectionData connData(packet);
            responder(connData);
            return true;
        } else if (packet.getPacketType() == loraconn::PacketType::DISCONN_REQ && disconnRequestResponder != nullptr) {
            loraconn::DisconnectionRequest disconnRequest(packet);
            disconnRequestResponder(disconnRequest);
            return true;
        } else {
            printing::error("Ignoring packet which is not a connection request", 1);
            return false;
        }
    } else {
        printing::error("Ignoring non-LoRaConn packet", 1);
        return false;
    }
}

bool radio::currentlyReceivingPacket()
{
    uint32_t flags = _radio.getIrqFlags();
    return flags == RADIOLIB_IRQ_PREAMBLE_DETECTED || flags == RADIOLIB_IRQ_RX_DONE;
}

bool radio::pollCompletedOperation()
{
    bool result = operationCompleted;
    operationCompleted = false;
    return result;
}
