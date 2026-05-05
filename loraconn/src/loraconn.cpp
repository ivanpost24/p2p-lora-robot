#include "loraconn.hpp"

static constexpr uint8_t PACKET_TYPE_MASK = 0b00000011;
static constexpr uint8_t PAIRS_MASK = 0b11111100;

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

bool loraconn::Advertisement::advertiserAddressMatches(const MACAddress &other) const
{
    return memcmp(data(), other.data(), other.size()) == 0;
}

void loraconn::Advertisement::setAdvertiserAddress(const MACAddress &advertiserAddress)
{
    std::copy(advertiserAddress.cbegin(), advertiserAddress.cend(), data());
}

uint16_t loraconn::Advertisement::getCentralRxWindow() const
{
    return (static_cast<uint16_t>(*data(7)) << 8) | *data(6);
}

void loraconn::Advertisement::setCentralRxWindow(uint16_t window)
{
    *data(6) = window & 0xFF;
    *data(7) = window >> 8;
}

uint8_t loraconn::Advertisement::getPayloadLength() const
{
    return *data(8);
}

void loraconn::Advertisement::setPayloadLength(uint8_t length)
{
    *data(8) = length;
}

uint16_t loraconn::Advertisement::getTimeOnAir() const
{
    return (static_cast<uint16_t>(*data(10)) << 8) | *data(9);
}

void loraconn::Advertisement::setTimeOnAir(uint16_t timeOnAir)
{
    *data(9) = timeOnAir & 0xFF;
    *data(10) = timeOnAir >> 8;
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

uint16_t loraconn::ConnectionRequest::getPeripheralRxWindow() const
{
    return (static_cast<uint16_t>(*data(9)) << 8) | *data(8);
}

void loraconn::ConnectionRequest::setPeripheralRxWindow(uint16_t windowSize)
{
    *data(8) = windowSize & 0xFF;
    *data(9) = windowSize >> 8;
}

uint16_t loraconn::ConnectionRequest::getFirstEventOffset() const
{
    return (static_cast<uint16_t>(*data(11)) << 8) | *data(10);
}

void loraconn::ConnectionRequest::setFirstEventOffset(uint16_t windowOffset)
{
    *data(10) = windowOffset & 0xFF;
    *data(11) = windowOffset >> 8;
}

uint8_t loraconn::ConnectionRequest::getFirstChannel() const
{
    return *data(12);
}

void loraconn::ConnectionRequest::setFirstChannel(uint8_t firstChannel)
{
    assert(firstChannel <= 112);
    *data(12) = firstChannel;
}

uint8_t loraconn::ConnectionRequest::getHopCount() const
{
    return *data(13);
}

void loraconn::ConnectionRequest::setHopCount(uint8_t hopCount)
{
    assert(hopCount >= 1 && hopCount <= 112);
    *data(13) = hopCount;
}

uint8_t loraconn::ConnectionRequest::getPayloadLength() const
{
    return *data(14);
}

void loraconn::ConnectionRequest::setPayloadLength(uint8_t length)
{
    *data(14) = length;
}

uint8_t loraconn::ConnectionRequest::getEventMessagePairs() const
{
    return _raw[2] >> 2;
}

void loraconn::ConnectionRequest::setEventMessagePairs(uint8_t pairs)
{
    assert(pairs < 64);
    _raw[2] = (_raw[2] & ~PAIRS_MASK) | (pairs << 2);
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
    return getDataLength() - dataHeaderLength;
}

void loraconn::ConnectionData::setPayloadLength(uint8_t payloadLength)
{
    setDataLength(dataHeaderLength + payloadLength);
}

void loraconn::ConnectionData::getPayload(uint8_t *out) const
{
    memcpy(out, data(3), getPayloadLength());
}

void loraconn::ConnectionData::setPayload(uint8_t *buf, uint8_t length)
{
    assert(length <= payloadCapacity);
    setPayloadLength(length);
    memcpy(data(3), buf, length);
}

const uint8_t *loraconn::ConnectionData::payload() const
{
    return data(3);
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
