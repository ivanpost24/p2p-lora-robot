# loraconn

Library providing data types for simple connection-style communication over LoRa. The full protocol specification is
provided below.

## 1. Physical layer

Devices shall use the LoRa communication standard. This should not be confused with LoRaWAN, which is the most popular
protocol using this physical layer protocol.

### 1.1. Channels

The protocol uses 114 channels in the 902-928 MHz ISM band numbered 0 to 113. Each channel has a bandwidth of 125 kHz,
and the spacing between each channel is 200 kHz. The frequency of channel $k$ can be calculated, in MHz, as follows:

    freq(k) = 903.0 + 0.2 * k

Channel 113 is reserved for advertising, and the remaining 113 channels are used for connections over frequency hopping
spread spectrum (FHSS).

### 1.2. Data rate parameters

All transmissions shall use a coding rate of 4/5. The spreading factor shall be agreed upon in advance.

## 2. Data link layer

LoRaConn is designed specifically for communication between a pre-configured central and peripheral device. It is
assumed that the central device is relatively powerful and thus can supply its own power. The peripheral device is
assumed to be a relatively low-power device.

### 2.1. Base header

Every frame sent over this protocol begins with the following header. Devices shall use the protocol identifier to
filter irrelevant packets sent by devices using other protocols.

| Length   | Name                | Description                                          |
| -------- | ------------------- | ---------------------------------------------------- |
| 2 octets | Protocol identifier | 0x1195, which identifies the protocol.               |
| 6 bits   | Special             | Special control flags, depending on the packet type. |
| 2 bits   | Packet type         | The type of the packet (see 2.1.1).                  |
| varies   | Rest of frame       |                                                      |

*Header length*: 3 octets

### 2.1.1. Packet types

The packet type field above may be one of the following. Each packet type is explained in further detail below.

| ID  | Type                   |
| --- | ---------------------- |
| 00  | Advertisement          |
| 01  | Connection request     |
| 10  | Connection data        |
| 11  | Disconnection request  |

### 2.2. Advertising

The peripheral device shall periodically send out advertisements on channel 119. Central devices shall continuously
listen on channel 119 for advertisements. The central device is assumed to already know the device address of the
peripheral device to which it wishes to connect (this information is provided out-of-band). The peripheral device
shall listen on channel 119 for at least the following durations:

| SF  | RX window duration |
| --- | ------------------ |
| 5   | 25 ms              |
| 6   | 30 ms              |
| 7   | 35 ms              |
| 8   | 45 ms              |
| 9   | 60 ms              |
| 10  | 85 ms              |
| 11  | 165 ms             |
| 12  | 270 ms             |

Advertisements contain a peripheral device address and a requested RX window length for the central device, which
should be chosen based on the spreading factor and other encoding parameters.

| Length   | Name               | Description                                                                                    |
| -------- | ------------------ | ---------------------------------------------------------------------------------------------- |
| 6 octets | Advertiser address | An uniquely identifying address for the peripheral device.                                     |
| 2 octets | Central RX window  | Time the central should wait after TX to the peripheral device for a response (little endian). |
| 1 octet  | Payload length     | Length of connection data payloads.                                                            |
| 2 octets | Time on air        | Estimated time on air for connection data payload transmissions (0.1 ms, little endian).       |

*Data length:* 11 octets; *Total length*: 14 octets

### 2.3. Connection request

Once the central device receives an advertisement from the correct peripheral device, it shall send a connection
request in response as soon as possible. The connection request will contain a requested RX window length for the
peripheral device, which should be chosen based on the spreading factor and other encoding parameters.

#### 2.3.1. Connection request special section

| Bit index (LSB–MSB) | Name                | Description                                   |
| ------------------- | ------------------- | --------------------------------------------  |
| 0–5                 | Event message pairs | Number of message pairs per connection event. |

#### 2.3.2. Connection request main section

| Length   | Name                  | Description                                                                                             |
| -------- | --------------------- | ------------------------------------------------------------------------------------------------------- |
| 6 octets | Advertiser address    | Address the advertiser used to identify itself.                                                         |
| 2 octets | Connection identifier | A randomly generated sequence to identify the connection.                                               |
| 2 octets | Peripheral RX window  | Time the peripheral should wait after TX to the central device for a response (little endian).          |
| 2 octets | First event offset    | Time the peripheral should wait after receiving this request until the first RX window (little endian). |
| 1 octet  | First channel         | Channel on which the first connection will occur (in range \[0, 112\]).                                 |
| 1 octet  | Hop count             | Amount to hop by between each channel. Each next channels is calculated as `c_next = (c + hop) % 113`.  |
| 1 octet  | Payload length        | Length of connection data payloads.                                                                     |

*Data length*: 15 octets; *Total length*: 18 octets

### 2.4. Connection data

Once a connection request is sent, the central device will send the first packet of the connection after the provided
window offset time, measured from the end of the packet transmission.

| Length   | Name                  | Description                                                       |
| -------- | --------------------- | ----------------------------------------------------------------- |
| 2 octets | Connection identifier | The same connection identifier sent in the connection request.    |
| varies   | Payload               |                                                                   |

*Header length:* 2 octets; *Total length*: 5 + (Payload length) octets

### 2.5. Disconnection request

Either device may send a disconnection request to terminate the connection.

| Length   | Name                  | Description                                                    |
| -------- | --------------------- | -------------------------------------------------------------- |
| 2 octets | Connection identifier | The same connection identifier sent in the connection request. |

*Data length:* 2 octets; *Total length*: 5 octets
