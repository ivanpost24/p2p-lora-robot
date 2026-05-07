# loraconn

Library providing data types for simple connection-style communication over LoRa. The full protocol specification is
provided below.

## 1. Physical layer

Devices shall use the LoRa communication standard. This should not be confused with LoRaWAN, which is the most popular
protocol using this physical layer protocol.

### 1.1. Channels

The protocol uses 32 channels in the 902-928 MHz ISM band numbered 0 to 31. Each channel has a bandwidth of 500 kHz,
and the spacing between each channel is 600 kHz. The frequency of channel $k$ can be calculated, in MHz, as follows:

    freq(k) = 908.4 + 0.6 * k

Channel 31 is reserved for advertising, and the remaining 30 channels are used for connections. Because the device is
transmitting at a bandwidth of 500 kHz, it is not necessary to hop channels, though this may be a nice feature to add
if the device is experiencing interference.

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
| 5 bits   | Special             | Special control flags, depending on the packet type. |
| 3 bits   | Packet type         | The type of the packet (see 2.1.1).                  |
| varies   | Rest of frame       |                                                      |

*Header length*: 3 octets

### 2.1.1. Packet types

The packet type field above may be one of the following. Each packet type is explained in further detail below.

| ID      | Type                    |
| ------- | ----------------------- |
| 000     | Advertisement           |
| 001–010 | Reserved for future use |
| 011     | Connection request      |
| 100     | Connection data         |
| 101–110 | Reserved for future use |
| 111     | Disconnection request   |

### 2.2. Advertising

The peripheral device shall periodically send out advertisements on channel 31. Central devices shall continuously
listen on channel 31 for advertisements. The central device is assumed to already know the device address of the
peripheral device to which it wishes to connect (this information is provided out-of-band).

Advertisements contain a peripheral device address and a requested RX window length for the central device, which
should be chosen based on the spreading factor and other encoding parameters.

| Length   | Name                      | Description                                                                                    |
| -------- | ------------------------- | ---------------------------------------------------------------------------------------------- |
| 6 octets | Peripheral device address | An uniquely identifying address for the peripheral device.                                     |
| 2 octets | Central RX window         | Time the central should wait after TX to the peripheral device for a response (little endian). |

*Data length:* 8 octets; *Total length*: 11 octets

### 2.3. Connection request

Once the central device receives an advertisement from the correct peripheral device, it shall send a connection
request in response as soon as possible. The connection request will contain a requested RX window length for the
peripheral device, which should be chosen based on the spreading factor and other encoding parameters.

#### 2.3.1. Connection request special section

| Bit index (LSB–MSB) | Name    | Description                                                   |
| ------------------- | ------- | ------------------------------------------------------------- |
| 0–4                 | Channel | The channel to use for communication, in the range \[0, 30\]. |

#### 2.3.2. Connection request main section

| Length   | Name                  | Description                                                                                             |
| -------- | --------------------- | ------------------------------------------------------------------------------------------------------- |
| 6 octets | Advertiser address    | Address the advertiser used to identify itself.                                                         |
| 2 octets | Connection identifier | A randomly generated sequence to identify the connection.                                               |
| 2 octets | Peripheral RX window  | Time the peripheral should wait after TX to the central device for a response (little endian).          |
| 2 octets | Window offset         | Time the peripheral should wait after receiving this request until the first RX window (little endian). |

*Data length*: 12 octets; *Total length*: 15 octets

### 2.4. Connection data

Once a connection request is sent, the central device will send the first packet of the connection after the provided
window offset time, measured from the end of the packet transmission.

| Length   | Name                  | Description                                                       |
| -------- | --------------------- | ----------------------------------------------------------------- |
| 2 octets | Connection identifier | The same connection identifier sent in the connection request.    |
| varies   | Payload               |                                                                   |

*Header length:* 2 octets; *Total length*: at least 5 octets.

### 2.5. Disconnection request

Either device may send a disconnection request to terminate the connection.

| Length   | Name                  | Description                                                    |
| -------- | --------------------- | -------------------------------------------------------------- |
| 2 octets | Connection identifier | The same connection identifier sent in the connection request. |

*Data length:* 2 octets; *Total length*: 5 octets
