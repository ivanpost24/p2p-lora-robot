#include "loraconn.hpp"

static constexpr uint8_t PACKET_TYPE_MASK = 0b00000111;
static constexpr uint8_t CHANNEL_MASK = 0b11111000;
static constexpr uint8_t SEQNO_MASK = 0b00001000;
static constexpr uint8_t NSQN_MASK = 0b00010000;

loraconn::PacketType loraconn::Packet::getPacketType() const
{
    return static_cast<PacketType>(_raw[2] & PACKET_TYPE_MASK);
}

void loraconn::Packet::setPacketType(PacketType packetType)
{
    _raw[2] = (_raw[2] & ~PACKET_TYPE_MASK) | (static_cast<uint8_t>(packetType) & PACKET_TYPE_MASK);
}

void loraconn::Advertisement::getAdvertiserAddress(MACAddress& out) const
{
    std::copy(data(), data(out.size()), out.begin());
}

void loraconn::Advertisement::setAdvertiserAddress(const MACAddress &advertiserAddress)
{
    std::copy(advertiserAddress.cbegin(), advertiserAddress.cend(), data());
}

void loraconn::ConnectionRequest::getAdvertiserAddress(MACAddress& out) const
{
    std::copy(data(), data(out.size()), out.begin());
}

bool loraconn::ConnectionRequest::advertiserAddressMatches(const MACAddress &other) const
{
    return memcmp(data(), other.data(), other.size()) == 0;
}

void loraconn::ConnectionRequest::setAdvertiserAddress(const MACAddress &advertiserAddress)
{
    std::copy(advertiserAddress.cbegin(), advertiserAddress.cend(), data());
}

void loraconn::ConnectionRequest::getConnectionIdentifier(ConnectionIdentifier &out) const
{
    std::copy(data(6), data(6 + out.size()), out.begin());
}

void loraconn::ConnectionRequest::setConnectionIdentifier(const ConnectionIdentifier &connectionIdentifier)
{
    std::copy(connectionIdentifier.cbegin(), connectionIdentifier.cend(), data(6));
}

uint16_t loraconn::ConnectionRequest::getWindowSize() const
{
    return (static_cast<uint16_t>(*data(9)) << 8) | *data(8);
}

void loraconn::ConnectionRequest::setWindowSize(uint16_t windowSize)
{
    *data(8) = windowSize & 0xFF;
    *data(9) = windowSize >> 8;
}

uint16_t loraconn::ConnectionRequest::getWindowOffset() const
{
    return (static_cast<uint16_t>(*data(11)) << 8) | *data(10);
}

void loraconn::ConnectionRequest::setWindowOffset(uint16_t windowOffset)
{
    *data(10) = windowOffset & 0xFF;
    *data(11) = windowOffset >> 8;
}

uint16_t loraconn::ConnectionRequest::getWindowInterval() const
{
    return (static_cast<uint16_t>(*data(13)) << 8) | *data(12);
}

void loraconn::ConnectionRequest::setWindowInterval(uint16_t windowInterval)
{
    *data(12) = windowInterval & 0xFF;
    *data(13) = windowInterval >> 8;
}

uint8_t loraconn::ConnectionRequest::getChannel() const
{
    return _raw[2] >> 3;
}

void loraconn::ConnectionRequest::setChannel(uint8_t firstChannel)
{
    assert(firstChannel <= 30);
    _raw[2] = (_raw[2] & ~CHANNEL_MASK) | (firstChannel << 3);
}

bool loraconn::ConnectionData::getSequenceNumber() const
{
    return static_cast<bool>((_raw[2] >> 3) & 0b1);
}

void loraconn::ConnectionData::setSequenceNumber(bool sequenceNumber)
{
    _raw[2] = (_raw[2] & ~SEQNO_MASK) & (static_cast<uint8_t>(sequenceNumber) << 3);
}

bool loraconn::ConnectionData::getNextExpectedSequenceNumber() const
{
    return static_cast<bool>((_raw[2] >> 4) & 0b1);
}

void loraconn::ConnectionData::setNextExpectedSequenceNumber(bool sequenceNumber)
{
    _raw[2] = (_raw[2] & ~NSQN_MASK) & (static_cast<uint8_t>(sequenceNumber) << 4);
}

void loraconn::ConnectionData::getConnectionIdentifier(ConnectionIdentifier &out) const
{
    std::copy(data(), data(out.size()), out.begin());
}

bool loraconn::ConnectionData::connectionIdentifierMatches(const ConnectionIdentifier &connId) const
{
    return memcmp(data(), connId.data(), connId.size()) == 0;
}

void loraconn::ConnectionData::setConnectionIdentifier(const ConnectionIdentifier &connectionIdentifier)
{
    std::copy(connectionIdentifier.cbegin(), connectionIdentifier.cend(), data());
}

uint8_t loraconn::ConnectionData::getPayloadLength() const
{
    return *data(2);
}

void loraconn::ConnectionData::setPayloadLength(uint8_t payloadLength)
{
    *data(2) = payloadLength;
}

void loraconn::ConnectionData::getPayload(uint8_t *out) const
{
    memcpy(out, data(3), getPayloadLength());
}

void loraconn::ConnectionData::setPayload(uint8_t *buf, uint8_t length)
{
    assert(length < payloadCapacity);
    setPayloadLength(length);
    memcpy(data(3), buf, length);
}

uint8_t *loraconn::ConnectionData::payload()
{
    return data(3);
}

void loraconn::DisconnectionRequest::getConnectionIdentifier(ConnectionIdentifier &out) const
{
    std::copy(data(), data(out.size()), out.begin());
}

bool loraconn::DisconnectionRequest::connectionIdentifierMatches(const ConnectionIdentifier &connId) const
{
    return memcmp(data(), connId.data(), connId.size()) == 0;
}

void loraconn::DisconnectionRequest::setConnectionIdentifier(const ConnectionIdentifier &connectionIdentifier)
{
    std::copy(connectionIdentifier.cbegin(), connectionIdentifier.cend(), data());
}

bool loraconn::isLoRaConnPacket(uint8_t *data)
{
    return memcmp(data, PROTOCOL_ID, 2) == 0;
}

loraconn::PacketType loraconn::getPacketType(uint8_t *data)
{
    return static_cast<PacketType>(data[2] & PACKET_TYPE_MASK);
}
