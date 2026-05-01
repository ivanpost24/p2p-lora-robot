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
static constexpr uint8_t SF = 7;
static constexpr int8_t TX_PWR = 20;
static constexpr uint8_t CR = 5;
static constexpr uint8_t SYNC_WORD = (uint8_t)0x34;
static constexpr uint16_t PREAMBLE = 8;

static constexpr loraconn::MACAddress macAddress = {0x58, 0x02, 0x34, 0x00, 0xfe, 0x54};
static constexpr unsigned long advRxWindow = 35000;
static constexpr unsigned long advEventLength = 1000000;
static constexpr unsigned long centralRxWindow = 25000;
static constexpr unsigned long txDelay = 500;

enum class State
{
    IDLE,
    ADV_TX,
    ADV_RX,
    ADV_SLEEP,
    CONN_STARTING,
    CONN_TX,
    CONN_RX,
    CONN_TX_SLEEP,
};

static volatile State state(State::IDLE);
static volatile bool operationCompleted = false;
static volatile uint16_t txError = RADIOLIB_ERR_NONE;
static volatile unsigned long eventStart = 0;
static volatile unsigned long connWindowOffset = 0;
static volatile unsigned long connRxWindow = 0;
static loraconn::ConnectionIdentifier connId;
static uint8_t buf[255];
static loraconn::Packet packet(255);

static int counter = 1;

static void setState(State _state)
{
    state = _state;
    Serial.print(F("State changed to "));
    switch (state) {
    case State::IDLE:
        Serial.println(F("IDLE"));
        break;
    case State::ADV_TX:
        Serial.println(F("ADV_TX"));
        break;
    case State::ADV_RX:
        Serial.println(F("ADV_RX"));
        break;
    case State::ADV_SLEEP:
        Serial.println(F("ADV_SLEEP"));
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
    return radio.startTransmit(packet.getRaw(), packet.getLength());
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
    } else if (!out.setRaw(buf, length)) {
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

void advertise()
{
    loraconn::Advertisement adv;
    adv.setAdvertiserAddress(macAddress);
    adv.setCentralRxWindow(static_cast<uint16_t>(centralRxWindow / 100));
    setState(State::ADV_TX);
    Serial.print(F("Starting advertisement ... "));
    eventStart = micros();
    txError = startTransmitting(adv);
}

void loop(void)
{
    uint16_t err;
    switch (state) {
    case State::IDLE:
        tune(loraconn::ADVERTISING_CHANNEL);
        advertise();
        return;
    case State::ADV_TX:
        if (operationCompleted) {
            operationCompleted = false;
            if (txError == RADIOLIB_ERR_NONE) {
                Serial.println(F("success"));
            } else {
                sendError("failed", txError);
            }
            setState(State::ADV_RX);
            err = radio.startReceive();
            eventStart = micros();
            if (err == RADIOLIB_ERR_NONE) {
                Serial.print(F("Waiting for response ... "));
            } else {
                sendError("Failed to start receiving", err);
                setState(State::IDLE);
            }
        }
        return;
    case State::ADV_RX:
        if (operationCompleted) {
            operationCompleted = false;
            if (readPacket(packet)) {
                if (packet.getPacketType() == loraconn::PacketType::CONN_REQ) {
                    loraconn::ConnectionRequest connRequest(packet);
                    if (connRequest.advertiserAddressMatches(macAddress)) {
                        eventStart = micros();
                        err = radio.finishReceive();
                        if (err != RADIOLIB_ERR_NONE) {
                            sendError("Failed to finish receiving", err);
                        }
                        connRequest.getConnectionIdentifier(connId);
                        connWindowOffset = static_cast<unsigned long>(connRequest.getWindowOffset()) * 100;
                        connRxWindow = static_cast<unsigned long>(connRequest.getPeripheralRxWindow()) * 100;
                        tune(connRequest.getChannel());
                        setState(State::CONN_STARTING);
                    } else {
                        Serial.println(F("Ignoring connection request to different device"));
                    }
                } else {
                    Serial.println(F("Ignoring packet of incorrect type"));
                }
            } else {
                Serial.println(F("Ignoring packet of incorrect protocol"));
            }
        } else if (micros() - eventStart >= advRxWindow && !currentlyReceivingPacket()) {
            Serial.println("No response to advertisement");
            setState(State::ADV_SLEEP);
        }
        return;
    case State::ADV_SLEEP:
        if (micros() - eventStart >= advEventLength) {
            advertise();
        }
        return;
    case State::CONN_STARTING:
        if (micros() - eventStart >= connWindowOffset) {
            setState(State::CONN_RX);
            err = radio.startReceive();
            eventStart = micros();
            if (err == RADIOLIB_ERR_NONE) {
                Serial.print(F("Waiting for first packet ... "));
            } else {
                sendError("Failed to start receiving", err);
                setState(State::IDLE);
            }
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

            setState(State::CONN_RX);
            err = radio.startReceive();
            if (err == RADIOLIB_ERR_NONE) {
                Serial.print(F("Waiting for next packet ... "));
            } else {
                sendError("Failed to start receiving", err);
                setState(State::IDLE);
            }
            eventStart = micros();
        }
        return;
    case State::CONN_RX:
        if (operationCompleted) {
            operationCompleted = false;
            if (readPacket(packet)) {
                if (packet.getPacketType() == loraconn::PacketType::CONN_DATA) {
                    loraconn::ConnectionData connData(packet);
                    if (connData.connectionIdentifierMatches(connId)) {
                        uint8_t length = connData.getPayloadLength();
                        if (length == 4) {
                            uint8_t payload[length];
                            connData.getPayload(payload);
                            counter = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
                            Serial.print(F("Received value: "));
                            Serial.println(counter);
                            err = radio.finishReceive();
                            operationCompleted = false;
                            if (err != RADIOLIB_ERR_NONE) {
                                sendError("Failed to finish receiving", err);
                            }
                            setState(State::CONN_TX_SLEEP);
                            eventStart = micros();
                        } else {
                            Serial.println(F("Invalid connection data payload"));
                        }
                    } else {
                        Serial.println(F("Ignoring data from a different connection"));
                    }
                } else if (packet.getPacketType() == loraconn::PacketType::DISCONN_REQ) {
                    loraconn::DisconnectionRequest disconnRequest(packet);
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
        } else if (micros() - eventStart >= connRxWindow && !currentlyReceivingPacket()) {
            Serial.println(F("Missed packet"));
            setState(State::IDLE);
        }
        return;
    case State::CONN_TX_SLEEP:
        if (micros() - eventStart >= txDelay) {
            setState(State::CONN_TX);
            counter += 1;
            uint8_t payload[4];
            payload[0] = counter >> 24;
            payload[1] = (counter >> 16) & 0xff;
            payload[2] = (counter >> 8) & 0xff;
            payload[3] = counter & 0xff;
            loraconn::ConnectionData connData(4);
            connData.setConnectionIdentifier(connId);
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