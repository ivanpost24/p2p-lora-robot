#ifndef WIOT_P2P_LORA_HPP
#define WIOT_P2P_LORA_HPP

#include <array>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>

namespace loraconn {

static constexpr uint8_t ADVERTISING_CHANNEL = 31;

constexpr float getChannelFrequency(uint8_t channel) {
    return 908.4f + 0.6f * channel;
}

constexpr float getChannelBandwidth(uint8_t channel) {
    return 500.0f;
}

/**
 * \brief Type alias representing a MAC Address.
 */
using MACAddress = std::array<uint8_t, 6>;

/**
 * \brief Type alias representing a Connection Identifier.
 */
using ConnectionIdentifier = std::array<uint8_t, 2>;

/**
 * \brief Enumeration of valid packet types.
 */
enum class PacketType : uint8_t
{

    /**
     * \brief Advertisement packet type.
     */
    ADVERT = 0b000,

    /**
     * \brief Connection Request packet type.
     */
    CONN_REQ = 0b011,

    /**
     * \brief Connection Data packet type.
     */
    CONN_DATA = 0b100,

    /**
     * \brief Disconnection Request packet type.
     */
    DISCONN_REQ = 0b111,
};

constexpr uint8_t PROTOCOL_ID[] = {0x11, 0x95};

bool isLoRaConnPacket(uint8_t *data);

struct Packet
{

    static constexpr uint8_t headerSize = 3;

    Packet(uint8_t dataSize)
    : _totalSize{headerSize + dataSize}
    {
        assert(dataSize <= UINT8_MAX - headerSize);
        _raw = new uint8_t[_totalSize];
        memcpy(_raw, PROTOCOL_ID, 2);
    }

    Packet(uint8_t *buf, uint8_t bufSize)
    : _totalSize{bufSize}, _dataSize{_dataSize - bufSize}, _raw{buf}
    {
        assert(bufSize >= headerSize);
    }

    Packet(const Packet& other)
    : _totalSize(other._totalSize), _dataSize(other._dataSize)
    {
        _raw = new uint8_t[other._totalSize];
        memcpy(_raw, other._raw, other._totalSize);
    }

    Packet(Packet&& other)
    : _totalSize{other._totalSize}, _dataSize{other._dataSize}
    {
        std::swap(this->_raw, other._raw);
    }

    Packet& operator=(Packet other)
    {
        swap(*this, other);  // copy-and-swap idiom
        return *this;
    }

    Packet& operator=(Packet&& other)
    {
        swap(*this, other);
        return *this;
    }

    uint8_t getTotalSize()
    {
        return _totalSize;
    }

    uint8_t getDataSize()
    {
        return _dataSize;
    }

    PacketType getPacketType();

    ~Packet() {
        delete[] this->_raw;
    }

    friend void swap(Packet& lhs, Packet& rhs)
    {
        using std::swap;

        swap(lhs._totalSize, rhs._totalSize);
        swap(lhs._dataSize, rhs._dataSize);
        swap(lhs._raw, rhs._raw);
    }

    inline const uint8_t *getData()
    {
        return _raw;
    }

protected:

    uint8_t _totalSize;
    uint8_t _dataSize;
    uint8_t *_raw;

    void setPacketType(PacketType packetType);

    inline uint8_t *data()
    {
        return _raw + headerSize;
    }

    inline uint8_t *data(uint8_t index)
    {
        assert(index < _dataSize);
        return _raw + headerSize + index;
    }

};

struct Advertisement : public Packet
{

    static constexpr PacketType type = PacketType::ADVERT;
    static constexpr uint8_t dataSize = 6;

    Advertisement() : Packet(dataSize)
    {
        this->setPacketType(type);
    }

    Advertisement(const Advertisement& other) : Packet(other)
    {}

    Advertisement(Advertisement&& other) : Packet(other)
    {}

    void getAdvertiserAddress(MACAddress& out);
    void setAdvertiserAddress(const MACAddress& advertiserAddress);

};

struct ConnectionRequest: public Packet
{

    static constexpr PacketType type = PacketType::CONN_REQ;
    static constexpr uint8_t dataSize = 13;

    ConnectionRequest() : Packet(dataSize)
    {
        this->setPacketType(type);
    }

    void getAdvertiserAddress(MACAddress& out);
    void setAdvertiserAddress(const MACAddress& advertiserAddress);

    void getConnectionIdentifier(ConnectionIdentifier& out);
    void setConnectionIdentifier(const ConnectionIdentifier& connectionIdentifier);

    uint8_t getWindowSize();
    void setWindowSize(uint8_t windowSize);

    uint16_t getWindowOffset();
    void setWindowOffset(uint16_t windowOffset);

    uint16_t getWindowInterval();
    void setWindowInterval(uint16_t windowInterval);

    uint8_t getChannel();
    void setChannel(uint8_t firstChannel);

};

struct ConnectionData : public Packet
{

    static constexpr PacketType type = PacketType::CONN_DATA;
    static constexpr uint8_t headerLength = 3;

    ConnectionData(uint8_t payloadCapacity)
    : Packet(headerLength + payloadCapacity), payloadCapacity{payloadCapacity}
    {
        this->setPacketType(type);
        this->setPayloadLength(payloadCapacity);
    }

    bool getSequenceNumber();
    void setSequenceNumber(bool sequenceNumber);

    bool getNextExpectedSequenceNumber();
    void setNextExpectedSequenceNumber(bool sequenceNumber);

    void getConnectionIdentifier(ConnectionIdentifier& out);
    void setConnectionIdentifier(const ConnectionIdentifier& connectionIdentifier);

    uint8_t getPayloadLength();
    void setPayloadLength(uint8_t payloadLength);

    void getPayload(uint8_t *out);
    void setPayload(uint8_t *buf, uint8_t length);
    uint8_t *payload();

private:

    uint8_t payloadCapacity;

};

struct DisconnectionRequest : public Packet
{

    static constexpr PacketType type = PacketType::DISCONN_REQ;
    static constexpr uint8_t size = 2;

    DisconnectionRequest()
    : Packet(size)
    {
        this->setPacketType(type);
    }

    void getConnectionIdentifier(ConnectionIdentifier& out);
    void setConnectionIdentifier(const ConnectionIdentifier& connectionIdentifier);

};

}

#endif
