/*
To successfully receive data, the following settings have to be the same
on both transmitter and receiver:
- carrier frequency
- bandwidth
- spreading factor
*/

// include the library
#include <RadioLib.h>
#include "controller.hpp"
#include "loraconn.hpp"
#include "base/printing.hpp"
#include "base/radio.hpp"
#include "base/timemark.hpp"

static constexpr uint8_t spreadingFactor = 8;
static constexpr int8_t TOO_MANY_MISSED_MESSAGES = 1;

static constexpr loraconn::MACAddress macAddress = {0x58, 0x02, 0x34, 0x00, 0xfe, 0x54};
static constexpr unsigned long advRxWindow = 500000;
static constexpr unsigned long advEventLength = 3000000;
static constexpr unsigned long centralRxWindow = 200000;
static constexpr unsigned long txDelay = 1000;

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
    DISCONN_TX,
};

static volatile uint8_t hopCount = 1;
static volatile uint8_t messagePairsPerEvent = 0;
static volatile unsigned long connFirstEventOffset = 0;
static volatile unsigned long connRxWindow = 0;
static volatile uint8_t messagesInEvent = 0;
static loraconn::ConnectionIdentifier connId;
static uint8_t firstChannel = 0;
static uint8_t missedMessages = 0;

static volatile State state(State::IDLE);
static loraconn::Packet packet(255);
static loraconn::ConnectionData txData(controller::peripheralPayloadLength);

static int16_t startAdvertising()
{
    if (radio::getChannel() != loraconn::ADVERTISING_CHANNEL) {
        int16_t err;
        err = radio::setChannel(loraconn::ADVERTISING_CHANNEL);
        if (err != RADIOLIB_ERR_NONE) {
            printing::error(F("Failed to switch to advertising channel"), err);
            return err;
        }
    }

    loraconn::Advertisement adv;
    adv.setAdvertiserAddress(macAddress);
    adv.setCentralRxWindow(static_cast<uint16_t>(centralRxWindow / 100));
    adv.setPayloadLength(controller::peripheralPayloadLength);
    adv.setTimeOnAir(static_cast<uint16_t>(
        radio::getApproximateTimeOnAir(controller::peripheralPayloadLength) / 100
    ));
    return radio::startTransmitting(adv);
}

static void configureConnection(const loraconn::ConnectionRequest& connRequest)
{
    connRequest.getConnectionIdentifier(connId);
    connRxWindow = static_cast<unsigned long>(connRequest.getPeripheralRxWindow()) * 100;
    connFirstEventOffset = static_cast<unsigned long>(connRequest.getFirstEventOffset()) * 100;
    firstChannel = connRequest.getFirstChannel();
    hopCount = connRequest.getHopCount();
    messagePairsPerEvent = connRequest.getEventMessagePairs();
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
        err = startAdvertising();
        if (err == RADIOLIB_ERR_NONE) {
            state = State::ADV_TX;
            timemark::mark();
            Serial.print(F("Transmitting advertisement ... "));
        } else {
            printing::error(F("Advertisement failed"), err);
            state = State::IDLE;
        }
        return;
    case State::ADV_TX:
        if (radio::pollCompletedOperation()) {
            Serial.println(F("success"));
            state = State::ADV_RX;
            err = radio::startReceiving();
            timemark::mark();
            if (err == RADIOLIB_ERR_NONE) {
                Serial.print(F("Waiting for response ... "));
            } else {
                printing::error("Failed to start receiving", err);
                state = State::IDLE;
            }
        } else if (timemark::timeSinceMark() >= 1000000) {
            Serial.println(F("timed out"));
            state = State::ADV_SLEEP;
        }
        return;
    case State::ADV_RX:
        if (radio::pollCompletedOperation()) {
            radio::readConnectionRequest(
                macAddress,
                [&](const loraconn::ConnectionRequest& connRequest) -> void {
                    Serial.println(F("Received connection request!"));
                    timemark::mark();
                    err = radio::finishReceiving();
                    if (err != RADIOLIB_ERR_NONE) {
                        printing::error(F("Failed to finish receiving"), err);
                    }
                    int ignore = controller::onConnectionRequested();
                    if (!ignore) {
                        configureConnection(connRequest);
                        state = State::CONN_STARTING;
                        err = prepareForFirstMessage();
                        if (err != RADIOLIB_ERR_NONE) {
                            printing::error(F("Could not prepare for connection"), err);
                        }
                    } else {
                        printing::error("Ignoring connection request", err);
                        state = State::IDLE;
                    }
                }
            );
        } else if (timemark::timeSinceMark() >= advRxWindow && !radio::currentlyReceivingPacket()) {
            Serial.println("No response to advertisement");
            state = State::ADV_SLEEP;
        }
        return;
    case State::ADV_SLEEP:
        if (timemark::timeSinceMark() >= advEventLength) {
            err = startAdvertising();
            if (err == RADIOLIB_ERR_NONE) {
                state = State::ADV_TX;
                timemark::mark();
                Serial.print(F("Transmitting advertisement ... "));
            } else {
                printing::error(F("Advertisement failed"), err);
                state = State::IDLE;
            }
        }
        return;
    case State::CONN_STARTING:
        if (timemark::timeSinceMark() >= connFirstEventOffset) {
            err = radio::startReceiving();
            if (err == RADIOLIB_ERR_NONE) {
                Serial.print(F("Waiting for first packet ... "));
                state = State::CONN_RX;
                timemark::mark();
            } else {
                printing::error("Failed to start receiving", err);
                state = State::IDLE;
            }

        }
        return;
    case State::CONN_TX:
        {
            bool proceed;
            if (radio::pollCompletedOperation()) {
                Serial.println(F("success"));
                proceed = true;
            } else if (timemark::timeSinceMark() >= centralRxWindow) {
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
            } else if (timemark::timeSinceMark() >= connRxWindow && !radio::currentlyReceivingPacket()) {
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
