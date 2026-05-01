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

PacketType getPacketType(uint8_t *data);

struct Packet
{

    static constexpr uint8_t headerLength = 3;

    Packet(uint8_t capacity)
    : _capacity(capacity)
    {
        assert(_capacity >= headerLength);
        _dataLength = capacity - headerLength;
        _raw = new uint8_t[_capacity];
        memcpy(_raw, PROTOCOL_ID, 2);
    }

    Packet(uint8_t *buf, uint8_t length)
    : _capacity{length}, _dataLength(length - headerLength), _raw{buf}
    {
        assert(length >= headerLength);
    }

    Packet(const Packet& other)
    : _capacity(other._capacity), _dataLength(other._dataLength)
    {
        _raw = new uint8_t[other._capacity];
        memcpy(_raw, other._raw, other._capacity);
    }

    Packet(Packet&& other)
    : _capacity{other._capacity}, _dataLength{other._dataLength}
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

    uint8_t getCapacity() const
    {
        return _capacity;
    }

    uint8_t getLength() const
    {
        return headerLength + _dataLength;
    }

    uint8_t getDataLength() const
    {
        return _dataLength;
    }

    void setDataLength(uint8_t length)
    {
        assert(headerLength + length <= _capacity);
        _dataLength = length;
    }

    PacketType getPacketType() const;

    ~Packet() {
        delete[] this->_raw;
    }

    friend void swap(Packet& lhs, Packet& rhs)
    {
        using std::swap;

        swap(lhs._capacity, rhs._capacity);
        swap(lhs._dataLength, rhs._dataLength);
        swap(lhs._raw, rhs._raw);
    }

    inline const uint8_t *getRaw() const
    {
        return _raw;
    }

    bool setRaw(uint8_t *buf, uint8_t length)
    {
        assert(length <= _capacity);
        assert(length >= headerLength);
        if (isLoRaConnPacket(buf)) {
            memcpy(_raw, buf, length);
            _dataLength = length - headerLength;
            return true;
        } else {
            return false;
        }
    }

protected:

    uint8_t _capacity;
    uint8_t _dataLength;
    uint8_t *_raw;

    void setPacketType(PacketType packetType);

    inline const uint8_t *data() const
    {
        return _raw + headerLength;
    }

    inline uint8_t *data()
    {
        return _raw + headerLength;
    }

    inline const uint8_t *data(uint8_t index) const
    {
        assert(index <= _dataLength);
        return _raw + headerLength + index;
    }

    inline uint8_t *data(uint8_t index)
    {
        assert(index <= _dataLength);
        return _raw + headerLength + index;
    }

};

struct Advertisement : public Packet
{

    static constexpr PacketType type = PacketType::ADVERT;
    static constexpr uint8_t dataLength = 8;

    Advertisement() : Packet(headerLength + dataLength)
    {
        this->setPacketType(type);
    }

    explicit Advertisement(const Packet& other) : Packet(other)
    {
        assert(getPacketType() == PacketType::ADVERT);
    }

    explicit Advertisement(Packet&& other) : Packet(std::move(other))
    {
        assert(getPacketType() == PacketType::ADVERT);
    }

    Advertisement(const Advertisement& other) : Packet(other)
    {}

    Advertisement(Advertisement&& other) : Packet(other)
    {}

    Advertisement& operator=(Packet other)
    {
        swap(*this, other);  // copy-and-swap idiom
        return *this;
    }

    Advertisement& operator=(Packet&& other)
    {
        swap(*this, other);
        return *this;
    }

    void getAdvertiserAddress(MACAddress& out) const;
    bool advertiserAddressMatches(const MACAddress& other) const;
    void setAdvertiserAddress(const MACAddress& advertiserAddress);

    uint16_t getCentralRxWindow() const;
    void setCentralRxWindow(uint16_t window);

};

struct ConnectionRequest: public Packet
{

    static constexpr PacketType type = PacketType::CONN_REQ;
    static constexpr uint8_t dataLength = 14;

    ConnectionRequest() : Packet(headerLength + dataLength)
    {
        this->setPacketType(type);
    }

    explicit ConnectionRequest(const Packet& other) : Packet(other)
    {
        assert(getPacketType() == PacketType::CONN_REQ);
    }

    explicit ConnectionRequest(Packet&& other) : Packet(std::move(other))
    {
        assert(getPacketType() == PacketType::CONN_REQ);
    }

    ConnectionRequest(const ConnectionRequest& other) : Packet(other)
    {}

    ConnectionRequest(ConnectionRequest&& other) : Packet(other)
    {}

    ConnectionRequest& operator=(Packet other)
    {
        swap(*this, other);  // copy-and-swap idiom
        return *this;
    }

    ConnectionRequest& operator=(Packet&& other)
    {
        swap(*this, other);
        return *this;
    }

    void getAdvertiserAddress(MACAddress& out) const;
    bool advertiserAddressMatches(const MACAddress& other) const;
    void setAdvertiserAddress(const MACAddress& advertiserAddress);

    void getConnectionIdentifier(ConnectionIdentifier& out) const;
    void setConnectionIdentifier(const ConnectionIdentifier& connectionIdentifier);

    uint16_t getPeripheralRxWindow() const;
    void setPeripheralRxWindow(uint16_t window);

    uint16_t getWindowOffset() const;
    void setWindowOffset(uint16_t windowOffset);

    uint8_t getChannel() const;
    void setChannel(uint8_t firstChannel);

};

struct ConnectionData : public Packet
{

    static constexpr PacketType type = PacketType::CONN_DATA;
    static constexpr uint8_t dataHeaderLength = 3;

    ConnectionData(uint8_t payloadCapacity)
    : Packet(headerLength + dataHeaderLength + payloadCapacity), payloadCapacity{payloadCapacity}
    {
        this->setPacketType(type);
        this->setPayloadLength(payloadCapacity);
    }

    explicit ConnectionData(const Packet& other) : Packet(other)
    {
        assert(getPacketType() == PacketType::CONN_DATA);
        assert(other.getDataLength() >= dataHeaderLength);
        this->payloadCapacity = other.getDataLength() - dataHeaderLength;
    }

    explicit ConnectionData(Packet&& other) : Packet(std::move(other))
    {
        assert(getPacketType() == PacketType::CONN_DATA);
        assert(this->getDataLength() >= dataHeaderLength);
        this->payloadCapacity = this->getDataLength() - dataHeaderLength;
    }

    ConnectionData(const ConnectionData& other) : Packet(other)
    {}

    ConnectionData(ConnectionData&& other) : Packet(other)
    {}

    void getConnectionIdentifier(ConnectionIdentifier& out) const;
    bool connectionIdentifierMatches(const ConnectionIdentifier& connId) const;
    void setConnectionIdentifier(const ConnectionIdentifier& connectionIdentifier);

    uint8_t getPayloadLength() const;
    void setPayloadLength(uint8_t payloadLength);

    void getPayload(uint8_t *out) const;
    void setPayload(uint8_t *buf, uint8_t length);
    uint8_t *payload();

private:

    uint8_t payloadCapacity;

};

struct DisconnectionRequest : public Packet
{

    static constexpr PacketType type = PacketType::DISCONN_REQ;
    static constexpr uint8_t dataLength = 2;

    DisconnectionRequest()
    : Packet(headerLength + dataLength)
    {
        this->setPacketType(type);
    }

    explicit DisconnectionRequest(const Packet& other) : Packet(other)
    {
        assert(getPacketType() == PacketType::DISCONN_REQ);
    }

    explicit DisconnectionRequest(Packet&& other) : Packet(std::move(other))
    {
        assert(getPacketType() == PacketType::DISCONN_REQ);
    }

    DisconnectionRequest(const DisconnectionRequest& other) : Packet(other)
    {}

    DisconnectionRequest(DisconnectionRequest&& other) : Packet(other)
    {}

    void getConnectionIdentifier(ConnectionIdentifier& out) const;
    bool connectionIdentifierMatches(const ConnectionIdentifier& connId) const;
    void setConnectionIdentifier(const ConnectionIdentifier& connectionIdentifier);

};

}

#endif
