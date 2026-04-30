/*
To successfully receive data, the following settings have to be the same
on both transmitter and receiver:
- carrier frequency
- bandwidth
- spreading factor
*/

// include the library
#include <RadioLib.h>
#include "loraconn.hpp"

/**************** Pin assignment for the Heltec V3 board ******************/
static constexpr int LORA_CS    = 8;      // Chip select pin
static constexpr int LORA_MOSI  = 10;
static constexpr int LORA_MISO  = 11;
static constexpr int LORA_SCK   = 9;
static constexpr int LORA_NRST  = 12;      // Reset pin
static constexpr int LORA_DIO1  = 14;      // DIO1 switch
static constexpr int LORA_BUSY  = 13;
static constexpr int BUTTON     = 0;

/****************LoRa parameters (you need to fill these params)******************/
static constexpr uint8_t SF = 9;
static constexpr int8_t TX_PWR = 20;
static constexpr uint8_t CR = 5;
static constexpr uint8_t SYNC_WORD = (uint8_t)0x34;
static constexpr uint16_t PREAMBLE = 8;

static constexpr loraconn::MACAddress peripheralAddress = {0x58, 0x02, 0x34, 0x00, 0xfe, 0x54};
static constexpr unsigned long connWindowOffset = 1000000;
static constexpr unsigned long connWindowInterval = 1000000;
static constexpr unsigned long connWindowSize = 100000;

enum class State
{
    IDLE,
    SCAN_TX,
    SCAN_RX,
    CONN_STARTING,
    CONN_TX,
    CONN_RX,
    CONN_TX_SLEEP,
    CONN_RX_SLEEP,
};

static volatile State state(State::IDLE);
static volatile bool operationCompleted = false;
static volatile uint8_t channel = loraconn::ADVERTISING_CHANNEL;
static volatile uint16_t txError = RADIOLIB_ERR_NONE;
static volatile unsigned long eventStart = 0;
static volatile bool lastSequenceNumber = 0;
static loraconn::ConnectionIdentifier connId;
static uint8_t buf[255];
static loraconn::Packet packet(255);

static int counter = 0;

static void setState(State _state)
{
    state = _state;
    Serial.print(F("State changed to "));
    switch (state) {
    case State::IDLE:
        Serial.println(F("IDLE"));
        break;
    case State::SCAN_TX:
        Serial.println(F("SCAN_TX"));
        break;
    case State::SCAN_RX:
        Serial.println(F("SCAN_RX"));
        break;
    case State::CONN_STARTING:
        Serial.println(F("CONN_STARTING"));
        break;
    case State::CONN_TX:
        Serial.println(F("CONN_TX"));
        break;
    case State::CONN_RX:
        Serial.println(F("CONN_RX"));
        break;
    case State::CONN_TX_SLEEP:
        Serial.println(F("CONN_TX_SLEEP"));
        break;
    case State::CONN_RX_SLEEP:
        Serial.println(F("CONN_RX_SLEEP"));
        break;
    default:
        Serial.println(F("UNKNOWN"));
        break;
    }
}

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void onOperationCompleted(void)
{
    operationCompleted = true;
}

void sendError(const char* message, int16_t state)
{
    Serial.printf("ERROR: %s (error code %d).\n", message, state);
}

// Helper function to print error messages
void terminateWithError(const char* message, int16_t state)
{
    Serial.printf("ERROR: %s (error code %d). Terminating.\n", message, state);
    while(true); // loop forever
}

SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_NRST, LORA_BUSY);

static uint16_t tune(uint8_t channel)
{
    int err;
    err = radio.setFrequency(loraconn::getChannelFrequency(channel));
    if (err != RADIOLIB_ERR_NONE) {
        Serial.print(F("Error tuning radio to channel "));
        Serial.print(channel);
        Serial.printf(" (error code %d)\n", err);
        return err;
    }

    err = radio.setBandwidth(loraconn::getChannelBandwidth(channel));
    if (err != RADIOLIB_ERR_NONE) {
        Serial.print(F("Error setting radio bandwidth (error code "));
        Serial.printf(" (error code %d)\n", err);
        return err;
    }

    Serial.print(F("Changed channel to "));
    Serial.println(channel);
    return 0;
}

static uint16_t startTransmitting(const loraconn::Packet& packet)
{
    return radio.startTransmit(packet.getData(), packet.getLength());
}

static bool readPacket(loraconn::Packet& out) {
    size_t length = radio.getPacketLength();
    if (length > 255) {
        sendError("Read packet was too long", RADIOLIB_ERR_PACKET_TOO_LONG);
        return false;
    }
    int err = radio.readData(buf, length);
    if (err != RADIOLIB_ERR_NONE) {
        Serial.printf("Failed to receive packet (error code %d)\n", err);
        return false;
    } else if (!out.setData(buf, length)) {
        Serial.println(F("Ignoring non-LoRaConn packet"));
        return false;
    } else {
        return true;
    }
}

static bool currentlyReceivingPacket() {
    uint32_t flags = radio.getIrqFlags();
    return flags == RADIOLIB_IRQ_PREAMBLE_DETECTED || flags == RADIOLIB_IRQ_RX_DONE;
}

void setup()
{
    Serial.begin(115200);

    // initialize SX1262 with default settings
    Serial.print(F("[SX1262] Initializing ... "));
    int err = radio.begin();
    if (err == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        terminateWithError("Failed to start radio", err);
    }

    err = radio.setSpreadingFactor(SF);
    if (err != RADIOLIB_ERR_NONE) {
        terminateWithError("SF initialization failed", err);
    }
    Serial.print("[SX1262] Spreading Factor:\t\t");
    Serial.println(SF);
    err = radio.setOutputPower(TX_PWR);
    if (err != RADIOLIB_ERR_NONE) {
        terminateWithError("Output Power initialization failed", err);
    }
    Serial.print("[SX1262] Transmit Power:\t\t");
    Serial.print(TX_PWR);
    Serial.println(F(" dBm"));

    err = radio.setCurrentLimit(140.0);
    if (err != RADIOLIB_ERR_NONE) {
        terminateWithError("Current limit intialization failed", err);
    }
    radio.setDio1Action(onOperationCompleted);
}

void loop(void)
{
    uint16_t err;
    switch (state) {
    case State::IDLE:
        tune(loraconn::ADVERTISING_CHANNEL);
        setState(State::SCAN_RX);
        err = radio.startReceive();
        if (err == RADIOLIB_ERR_NONE) {
            Serial.print(F("Waiting for advertisement ... "));
        } else {
            sendError("Failed to start receiving", err);
            setState(State::IDLE);
        }
        return;
    case State::SCAN_TX:
        if (operationCompleted) {
            operationCompleted = false;
            if (txError == RADIOLIB_ERR_NONE) {
                Serial.println(F("success"));
            } else {
                sendError("failed", txError);
            }
            setState(State::CONN_STARTING);
            eventStart = micros();
        }
        return;
    case State::SCAN_RX:
        if (operationCompleted) {
            operationCompleted = false;
            if (readPacket(packet)) {
                if (packet.getPacketType() == loraconn::PacketType::ADVERT) {
                    loraconn::Advertisement advertisement{std::move(packet)};
                    if (advertisement.advertiserAddressMatches(peripheralAddress)) {
                        Serial.println(F("Found peripheral device!"));
                        eventStart = micros();
                        err = radio.finishReceive();
                        operationCompleted = false;
                        if (err != RADIOLIB_ERR_NONE) {
                            sendError("Failed to finish receiving", err);
                        }
                        long randomId = random();
                        connId.at(0) = (randomId >> 8) & 0xff;
                        connId.at(1) = randomId & 0xff;
                        channel = random() & 0b11111;
                        loraconn::ConnectionRequest connRequest;
                        connRequest.setAdvertiserAddress(peripheralAddress);
                        connRequest.setChannel(channel);
                        connRequest.setConnectionIdentifier(connId);
                        connRequest.setWindowOffset(connWindowOffset / 100);
                        connRequest.setWindowInterval(connWindowInterval / 100);
                        connRequest.setWindowSize(connWindowSize / 100);

                        setState(State::SCAN_TX);
                        txError = startTransmitting(connRequest);
                        Serial.print(F("Starting connection request ... "));
                    } else {
                        Serial.println(F("Ignoring advertisement from incorrect device"));
                    }
                } else {
                    Serial.println(F("Ignoring packet of incorrect type"));
                }
            } else {
                Serial.println(F("Ignoring packet of incorrect protocol"));
            }
        }
        return;
    case State::CONN_STARTING:
        if (micros() - eventStart >= connWindowOffset) {
            setState(State::CONN_TX);
            uint8_t payload[4];
            memset(payload, 0, 4);
            loraconn::ConnectionData connData(4);
            connData.setConnectionIdentifier(connId);
            connData.setSequenceNumber(0);
            connData.setNextExpectedSequenceNumber(0);
            connData.setPayload(payload, 4);
            txError = startTransmitting(connData);
        }
        return;
    case State::CONN_TX:
        if (operationCompleted) {
            operationCompleted = false;
            if (txError == RADIOLIB_ERR_NONE) {
                Serial.println(F("success"));
            } else {
                sendError("failed", txError);
            }
            setState(State::CONN_TX_SLEEP);
        }
        return;
    case State::CONN_RX:
        if (operationCompleted) {
            operationCompleted = false;
            if (readPacket(packet)) {
                if (packet.getPacketType() == loraconn::PacketType::CONN_DATA) {
                    loraconn::ConnectionData connData{std::move(packet)};
                    if (connData.connectionIdentifierMatches(connId)) {
                        uint8_t length = connData.getPayloadLength();
                        if (length == 4) {
                            uint8_t payload[length];
                            connData.getPayload(payload);
                            counter = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
                            Serial.print(F("Received value: "));
                            Serial.println(counter);
                            setState(State::CONN_RX_SLEEP);
                            err = radio.finishReceive();
                            operationCompleted = false;
                            if (err != RADIOLIB_ERR_NONE) {
                                sendError("Failed to finish receiving", err);
                            }
                        } else {
                            Serial.println(F("Invalid connection data payload"));
                        }

                    } else {
                        Serial.println(F("Ignoring data from a different connection"));
                    }
                } else if (packet.getPacketType() == loraconn::PacketType::DISCONN_REQ) {
                    loraconn::DisconnectionRequest disconnRequest{std::move(packet)};
                    if (disconnRequest.connectionIdentifierMatches(connId)) {
                        Serial.println("Disconnecting");
                        setState(State::IDLE);
                    } else {
                        Serial.println(F("Ignoring data from a different connection"));
                    }
                } else {
                    Serial.println(F("Ignoring packet of an incorrect type"));
                }
            } else {
                Serial.println(F("Ignoring packet of incorrect protocol"));
            }
        } else if (micros() - eventStart >= connWindowSize && !currentlyReceivingPacket()) {
            Serial.println(F("Missed packet"));
            setState(State::CONN_RX_SLEEP);
        }
        return;
    case State::CONN_TX_SLEEP:
        if (micros() - eventStart >= connWindowInterval) {
            setState(State::CONN_RX);
            err = radio.startReceive();
            eventStart = micros();
            if (err == RADIOLIB_ERR_NONE) {
                Serial.print(F("Waiting for next packet ... "));
            } else {
                sendError("Failed to start receiving", err);
                setState(State::IDLE);
            }
        }
        return;
    case State::CONN_RX_SLEEP:
        if (micros() - eventStart >= connWindowInterval) {
            setState(State::CONN_TX);
            counter += 1;
            uint8_t payload[4];
            payload[0] = counter >> 24;
            payload[1] = (counter >> 16) & 0xff;
            payload[2] = (counter >> 8) & 0xff;
            payload[3] = counter & 0xff;
            loraconn::ConnectionData connData(4);
            connData.setConnectionIdentifier(connId);
            connData.setSequenceNumber(lastSequenceNumber);
            connData.setNextExpectedSequenceNumber(lastSequenceNumber);
            connData.setPayload(payload, 4);
            eventStart = micros();
            txError = startTransmitting(connData);
            Serial.print(F("Started transmitting ... "));
        }
        return;
    default:
        Serial.println("Invalid state");
        while (true);
    }
}