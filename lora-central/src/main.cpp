/*
To successfully receive data, the following settings have to be the same
on both transmitter and receiver:
- carrier frequency
- bandwidth
- spreading factor
*/

#include <RadioLib.h>
#include "controller.hpp"
#include "loraconn.hpp"
#include "base/printing.hpp"
#include "base/radio.hpp"
#include "base/timemark.hpp"

static constexpr uint8_t spreadingFactor = 8;
static constexpr int8_t TOO_MANY_MISSED_MESSAGES = 1;

static constexpr loraconn::MACAddress peripheralAddress = {0x58, 0x02, 0x34, 0x00, 0xfe, 0x54};
static constexpr unsigned long connFirstEventOffset = 200000;
static constexpr unsigned long peripheralRxWindow = 200000;
static constexpr unsigned long txDelay = 1000;

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

static volatile uint8_t hopCount = 1;
static volatile uint8_t messagePairsPerEvent = 0;
static volatile unsigned long rxWindow = 0;
static volatile uint8_t messagesInEvent = 0;
static loraconn::ConnectionIdentifier connId;
static uint8_t firstChannel = 0;
static uint8_t missedMessages = 0;

static volatile State state(State::IDLE);
static loraconn::Packet packet(255);
static loraconn::ConnectionData txData(controller::centralPayloadLength);

static uint8_t getEventMessagePairs(unsigned long peripheralTimeOnAir, uint8_t centralPayloadLength)
{
    RadioLibTime_t timeOnAir = peripheralTimeOnAir + radio::getApproximateTimeOnAir(centralPayloadLength);
    Serial.printf("Estimated time on air: %d\n", timeOnAir);
    return static_cast<uint8_t>(320000.0f / timeOnAir);
}

static int16_t startScanning()
{
    int16_t err;
    err = radio::setChannel(loraconn::ADVERTISING_CHANNEL);
    if (err != RADIOLIB_ERR_NONE) {
        printing::error(F("Failed to switch to advertising channel"), err);
        return err;
    }

    return radio::startReceiving();
}

static void configureConnection(const loraconn::Advertisement& advertisement) {
    rxWindow = static_cast<unsigned long>(advertisement.getCentralRxWindow()) * 100;
    long randomId = random();
    connId.at(0) = (randomId >> 8) & 0xff;
    connId.at(1) = randomId & 0xff;
    do {
        firstChannel = random() & 0b111111;
    } while (firstChannel >= loraconn::ADVERTISING_CHANNEL);
    do {
        hopCount = random() & 0b111111;
    } while (hopCount < 1 || hopCount > 58);
    messagePairsPerEvent = getEventMessagePairs(
        static_cast<unsigned long>(advertisement.getTimeOnAir()) * 100,
        controller::centralPayloadLength
    );
    if (messagePairsPerEvent == 0) {
        Serial.println(F("Messages take too long to send. Aborting."));
        while (true);
    }
    Serial.printf("Messages per event: %d\n", 2 * static_cast<uint16_t>(messagePairsPerEvent));
}

static int16_t startConnectionRequest(const loraconn::Advertisement& advertisement)
{
    loraconn::ConnectionRequest connRequest;
    connRequest.setAdvertiserAddress(peripheralAddress);
    connRequest.setConnectionIdentifier(connId);
    connRequest.setPeripheralRxWindow(static_cast<uint16_t>(peripheralRxWindow / 100));
    connRequest.setFirstEventOffset(static_cast<uint16_t>(connFirstEventOffset / 100));
    connRequest.setFirstChannel(firstChannel);
    connRequest.setHopCount(hopCount);
    connRequest.setPayloadLength(controller::centralPayloadLength);
    connRequest.setEventMessagePairs(messagePairsPerEvent);

    return radio::startTransmitting(connRequest);
}

static int16_t prepareForFirstMessage()
{
    missedMessages = 0;
    messagesInEvent = 0;
    return radio::setChannel(firstChannel);
}

static int16_t prepareForNextMessage()
{
    if (missedMessages >= messagePairsPerEvent) {
        return TOO_MANY_MISSED_MESSAGES;
    }
    if (++messagesInEvent >= 2 * static_cast<uint16_t>(messagePairsPerEvent)) {
        missedMessages = 0;
        messagesInEvent = 0;
        return radio::setChannel(loraconn::getNextChannel(radio::getChannel(), hopCount));
    } else {
        return RADIOLIB_ERR_NONE;
    }
}

static int processPayload(const loraconn::ConnectionData& data)
{
    uint8_t length = data.getPayloadLength();
    uint8_t payload[length];
    data.getPayload(payload);
    return controller::onReceive(payload, length);
}

static int preparePayload(loraconn::ConnectionData& data)
{
    data.setConnectionIdentifier(connId);
    return controller::prepareTxPacket(txData.payload());
}

static int16_t startDisconnectionRequest()
{
    loraconn::DisconnectionRequest disconnRequest;
    disconnRequest.setConnectionIdentifier(connId);
    return radio::startTransmitting(disconnRequest);
}

void setup()
{
    Serial.begin(115200);

    int16_t err = radio::setup(spreadingFactor);
    if (err != RADIOLIB_ERR_NONE) {
        Serial.println(F("Terminating..."));
        while (true);
    }

    controller::setup();
}

void loop(void)
{
    int16_t err;
    switch (state) {
    case State::IDLE:
        err = startScanning();
        if (err == RADIOLIB_ERR_NONE) {
            Serial.print(F("Waiting for advertisement ... "));
            state = State::SCAN_RX;
        } else {
            printing::error("Failed to start receiving", err);
            state = State::IDLE;
        }
        return;
    case State::SCAN_RX:
        if (radio::pollCompletedOperation()) {
            radio::readAdvertisement(
                peripheralAddress,
                [&](const loraconn::Advertisement& advertisement) -> void {
                    Serial.println(F("Found peripheral device!"));
                    err = radio::finishReceiving();
                    if (err != RADIOLIB_ERR_NONE) {
                        printing::error(F("Failed to finish receiving"), err);
                    }
                    int ignore = controller::onPeripheralDetected();
                    if (!ignore) {
                        configureConnection(advertisement);
                        err = startConnectionRequest(advertisement);
                        if (err == RADIOLIB_ERR_NONE) {
                            Serial.print(F("Starting connection request ... "));
                            state = State::SCAN_TX;
                            timemark::mark();
                        } else {
                            printing::error(F("Failed to transmit connection request"), err);
                            state = State::IDLE;
                        }
                    } else {
                        Serial.printf("Not attempting to connect (code %d)", ignore);
                        state = State::IDLE;
                    }
                }
            );
        }
        return;
    case State::SCAN_TX:
        if (radio::pollCompletedOperation()) {
            Serial.println(F("success"));
            timemark::mark();
            err = prepareForFirstMessage();
            if (err == RADIOLIB_ERR_NONE) {
                state = State::CONN_STARTING;
            } else {
                printing::error(F("Disconnecting because hopping channels failed"), err);
                state = State::IDLE;
            }
        } else if (timemark::timeSinceMark() >= 1000000) {
            Serial.println(F("timed out"));
            state = State::IDLE;
        }
        return;
    case State::CONN_STARTING:
        if (timemark::timeSinceMark() >= connFirstEventOffset) {
            state = State::CONN_TX_SLEEP;
            timemark::mark();
        }
        return;
    case State::CONN_TX:
        {
            bool proceed;
            if (radio::pollCompletedOperation()) {
                Serial.println(F("success"));
                proceed = true;
            } else if (timemark::timeSinceMark() >= peripheralRxWindow) {
                Serial.println(F("timed out"));
                proceed = true;
            } else {
                proceed = false;
            }
            if (proceed) {
                err = prepareForNextMessage();
                if (err == RADIOLIB_ERR_NONE) {
                    err = radio::startReceiving();
                    if (err == RADIOLIB_ERR_NONE) {
                        Serial.print(F("Waiting for next packet ... "));
                        state = State::CONN_RX;
                    } else {
                        printing::error(F("Failed to start receiving"), err);
                        state = State::IDLE;
                    }
                    timemark::mark();
                } else {
                    if (err == TOO_MANY_MISSED_MESSAGES) {
                        printing::error(F("Disconnecting because there were too many missed messages"), err);
                    } else {
                        printing::error(F("Disconnecting because hopping channels failed"), err);
                    }
                    state = State::IDLE;
                }
            }
        }
        return;
    case State::CONN_RX:
        {
            bool proceed;
            if (radio::pollCompletedOperation()) {
                proceed = radio::readConnectionData(
                    connId,
                    [&](const loraconn::ConnectionData& data) -> void {
                        int status = processPayload(data);
                        if (status != 0) {
                            printing::error(F("Invalid connection data payload"), err);
                        }
                    }
                );
            } else if (timemark::timeSinceMark() >= rxWindow && !radio::currentlyReceivingPacket()) {
                Serial.println(F("Missed packet"));
                missedMessages++;
                proceed = true;
                state = State::CONN_TX_SLEEP;
            }
            if (proceed) {
                err = radio::finishReceiving();
                if (err != RADIOLIB_ERR_NONE) {
                    printing::error(F("Failed to finish receiving"), err);
                }
                state = State::CONN_TX_SLEEP;
                timemark::mark();
                err = prepareForNextMessage();
                if (err != RADIOLIB_ERR_NONE) {
                    if (err == TOO_MANY_MISSED_MESSAGES) {
                        printing::error(F("Disconnecting because there were too many missed messages"), err);
                    } else {
                        printing::error(F("Disconnecting because hopping channels failed"), err);
                    }
                    state = State::IDLE;
                }
            }
        }
        return;
    case State::CONN_TX_SLEEP:
        if (timemark::timeSinceMark() >= txDelay) {
            int disconnecting = preparePayload(txData);
            if (!disconnecting) {
                state = State::CONN_TX;
                timemark::mark();
                err = radio::startTransmitting(txData);
                if (err != RADIOLIB_ERR_NONE) {
                    printing::error(F("Failed to start transmitting"), err);
                    state = State::IDLE;
                }
                Serial.print(F("Started transmitting ... "));
            } else {
                err = startDisconnectionRequest();
                if (err == RADIOLIB_ERR_NONE) {
                    state = State::DISCONN_TX;
                    Serial.print(F("Disconnecting ... "));
                    timemark::mark();
                } else {
                    printing::error(F("Failed to send disconnection request"), err);
                    state = State::IDLE;
                }
            }
        }
        return;
    case State::DISCONN_TX:
        if (radio::pollCompletedOperation()) {
            state = State::IDLE;
        }
        return;
    default:
        Serial.println("Invalid state");
        while (true);
    }
}
