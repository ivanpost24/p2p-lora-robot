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

/// @brief Perform additional setup after the radio has been configured.
///
/// This function is called at the end of the Arduino `setup()` function after
/// the radio has been configured.
extern void performSetup();

/// @brief Prepare to initiate a connection with the peripheral device.
///
/// This function is called right before transmitting a connection request.
/// Return 0 at the end of the function to continue connecting. If you
/// return another value instead, the device will not attempt to connect.
/// @return `0` if you wish to continue connecting, and a different if you wish
///     to not attempt to connect.
extern int onPeripheralDetected();

/// @brief Prepare the next packet for transmission.
///
/// This function is called right before transmitting a packet. It should
/// modify the provided buffer and length for the data you wish to transmit.
/// Return 0 at the end of the function to perform a transmission. If you
/// return another value instead, the device will terminate the connection.
/// @param data Output buffer which will contain the data you wish to transmit.
/// @param capacity Capacity of the output buffer.
/// @param len Length of the output buffer.
/// @return `0` if you wish to continue the connection, and a different value
///     if you wish to terminate it.
extern int prepareTxPacket(uint8_t *data, uint8_t capacity, uint8_t& len);

/// @brief Process a received packet.
///
/// This function is called right after receiving a packet and provides the received
/// data as input. Return true at the end of the function to indicate that the device
/// should stop listening for data (because the received data is valid). Return false to
/// ask the device to continue listening until timeout.
/// @param data Input buffer which contains the data received.
/// @param len Length of the input buffer.
/// @return `0` if you want the device to stop listening and switch to transmission,
///     another value if you want the device to continue listening.
extern int onReceive(const uint8_t *data, uint8_t len);

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
static constexpr uint8_t SF = 10;
static constexpr int8_t TX_PWR = 22;
static constexpr uint8_t CR = 5;
static constexpr uint8_t SYNC_WORD = (uint8_t)0x34;
static constexpr uint16_t PREAMBLE = 8;

static constexpr loraconn::MACAddress peripheralAddress = {0x58, 0x02, 0x34, 0x00, 0xfe, 0x54};
static constexpr unsigned long connWindowOffset = 150000;
static constexpr unsigned long peripheralRxWindow = 150000;
static constexpr unsigned long txDelay = 500;
static constexpr uint8_t missedPacketsTolerance = 4;

enum class State
{
    IDLE,
    SCAN_TX,
    SCAN_RX,
    CONN_STARTING,
    CONN_TX,
    CONN_RX,
    CONN_TX_SLEEP,
    DISCONN_TX,
};

static volatile State state(State::IDLE);
static volatile bool operationCompleted = false;
static volatile uint16_t txError = RADIOLIB_ERR_NONE;
static volatile uint8_t channel = loraconn::ADVERTISING_CHANNEL;
static volatile unsigned long rxWindow = 0;
static volatile unsigned long eventStart = 0;
static loraconn::ConnectionIdentifier connId;
static uint8_t buf[255];
static loraconn::Packet packet(255);
static loraconn::ConnectionData txData(249);
static uint8_t missedPackets = 0;

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
    return RADIOLIB_ERR_NONE;
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
        sendError("Failed to receive packet", err);
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

    performSetup();
}

void loop(void)
{
    uint16_t err;
    switch (state) {
    case State::IDLE:
        tune(loraconn::ADVERTISING_CHANNEL);
        state = State::SCAN_RX;
        err = radio.startReceive();
        if (err == RADIOLIB_ERR_NONE) {
            Serial.print(F("Waiting for advertisement ... "));
        } else {
            sendError("Failed to start receiving", err);
            state = State::IDLE;
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
            state = State::CONN_STARTING;
            eventStart = micros();
        }
        return;
    case State::SCAN_RX:
        if (operationCompleted) {
            operationCompleted = false;
            if (readPacket(packet)) {
                if (packet.getPacketType() == loraconn::PacketType::ADVERT) {
                    loraconn::Advertisement advertisement(packet);
                    if (advertisement.advertiserAddressMatches(peripheralAddress)) {
                        Serial.println(F("Found peripheral device!"));
                        eventStart = micros();
                        err = radio.finishReceive();
                        operationCompleted = false;
                        if (err != RADIOLIB_ERR_NONE) {
                            sendError("Failed to finish receiving", err);
                        }
                        err = onPeripheralDetected();
                        if (err == 0) {
                            rxWindow = static_cast<unsigned long>(advertisement.getCentralRxWindow()) * 100;
                            long randomId = random();
                            connId.at(0) = (randomId >> 8) & 0xff;
                            connId.at(1) = randomId & 0xff;
                            do {
                                channel = random() & 0b11111;
                            } while (channel == 31);
                            loraconn::ConnectionRequest connRequest;
                            connRequest.setAdvertiserAddress(peripheralAddress);
                            connRequest.setChannel(channel);
                            connRequest.setConnectionIdentifier(connId);
                            connRequest.setWindowOffset(static_cast<uint16_t>(connWindowOffset / 100));
                            connRequest.setPeripheralRxWindow(static_cast<uint16_t>(peripheralRxWindow / 100));

                            state = State::SCAN_TX;
                            txError = startTransmitting(connRequest);
                            Serial.print(F("Starting connection request ... "));
                        } else {
                            sendError("Not attempting to connect", err);
                            state = State::IDLE;
                        }
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
            err = tune(channel);
            if (err == RADIOLIB_ERR_NONE) {
                state = State::CONN_TX_SLEEP;
                eventStart = micros();
            } else {
                state = State::IDLE;
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
            state = State::CONN_RX;
            err = radio.startReceive();
            if (err == RADIOLIB_ERR_NONE) {
                Serial.print(F("Waiting for next packet ... "));
            } else {
                sendError("Failed to start receiving", err);
                state = State::IDLE;
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
                        uint8_t payload[length];
                        connData.getPayload(payload);
                        err = onReceive(payload, length);
                        if (err == 0) {
                            err = radio.finishReceive();
                            operationCompleted = false;
                            missedPackets = 0;
                            if (err != RADIOLIB_ERR_NONE) {
                                sendError("Failed to finish receiving", err);
                            }
                            state = State::CONN_TX_SLEEP;
                            eventStart = micros();
                        } else {
                            sendError("Invalid connection data payload", err);
                        }
                    } else {
                        Serial.println(F("Ignoring data from a different connection"));
                    }
                } else if (packet.getPacketType() == loraconn::PacketType::DISCONN_REQ) {
                    loraconn::DisconnectionRequest disconnRequest(packet);
                    if (disconnRequest.connectionIdentifierMatches(connId)) {
                        Serial.println("Disconnecting");
                        state = State::IDLE;
                    } else {
                        Serial.println(F("Ignoring data from a different connection"));
                    }
                } else {
                    Serial.println(F("Ignoring packet of an incorrect type"));
                }
            } else {
                Serial.println(F("Ignoring packet of incorrect protocol"));
            }
        } else if (micros() - eventStart >= rxWindow && !currentlyReceivingPacket()) {
            Serial.println(F("Missed packet"));
            if (++missedPackets > missedPacketsTolerance) {
                Serial.println(F("Disconnecting"));
                state = State::IDLE;
            } else {
                state = State::CONN_TX_SLEEP;
            }
        }
        return;
    case State::CONN_TX_SLEEP:
        if (micros() - eventStart >= txDelay) {
            txData.setConnectionIdentifier(connId);
            uint8_t length;
            err = prepareTxPacket(txData.payload(), 249, length);
            if (err == 0) {
                state = State::CONN_TX;
                txData.setPayloadLength(length);
                eventStart = micros();
                txError = startTransmitting(txData);
                Serial.print(F("Started transmitting ... "));
            } else {
                state = State::DISCONN_TX;
                loraconn::DisconnectionRequest disconnRequest;
                disconnRequest.setConnectionIdentifier(connId);
                eventStart = micros();
                txError = startTransmitting(disconnRequest);
                Serial.print(F("Disconnecting ... "));
            }
        }
        return;
    case State::DISCONN_TX:
        if (operationCompleted) {
            operationCompleted = false;
            if (txError == RADIOLIB_ERR_NONE) {
                Serial.println(F("success"));
            } else {
                sendError("failed", txError);
            }

            state = State::IDLE;
        }
        return;
    default:
        Serial.println("Invalid state");
        while (true);
    }
}