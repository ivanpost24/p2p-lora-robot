#ifndef WIOT_CONTROLLER_HPP
#define WIOT_CONTROLLER_HPP

#include <stdint.h>

namespace controller {

/// @brief Perform additional setup after the radio has been configured.
///
/// This function is called at the end of the Arduino `setup()` function after
/// the radio has been configured.
void setup();

/// @brief Prepare to initiate a connection with the peripheral device.
///
/// This function is called right before transmitting a connection request.
/// Return 0 at the end of the function to continue connecting. If you
/// return another value instead, the device will not attempt to connect.
/// @return `0` if you wish to continue connecting, and a different if you wish
///     to not attempt to connect.
int onPeripheralDetected();

/// @brief Prepare the next packet for transmission.
///
/// This function is called right before transmitting a packet. It should
/// modify the provided buffer to contain the data you wish to transmit.
/// Return 0 at the end of the function to perform a transmission. If you
/// return another value instead, the device will terminate the connection.
/// @param data Output buffer which will contain the data you wish to transmit.
/// @return `0` if you wish to continue the connection, and a different value
///     if you wish to terminate it.
int prepareTxPacket(uint8_t *data);

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
int onReceive(const uint8_t *data, uint8_t len);

/// @brief The length of a payload sent from the central (this) device.
///
/// The protocol requires all packets sent by devices to be of the same length.
/// Specify that length here.
extern const uint8_t centralPayloadLength;

}

#endif