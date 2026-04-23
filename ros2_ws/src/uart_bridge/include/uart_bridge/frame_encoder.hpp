#ifndef UART_BRIDGE_FRAME_ENCODER_HPP
#define UART_BRIDGE_FRAME_ENCODER_HPP

#include <cstdint>
#include <vector>
#include "uart_protocol.h"

class FrameEncoder {
public:
  FrameEncoder();
  ~FrameEncoder() = default;

  // Encode servo control frame from command items
  std::vector<uint8_t> EncodeServoControl(const ServoCmdItem* items, size_t count);

  // Helper: encode single servo command
  std::vector<uint8_t> EncodeSingleServo(uint8_t servo_id, int16_t angle_x10,
                                          uint16_t duration_ms = 100);

private:
  std::vector<uint8_t> BuildFrame(uint8_t cmd_id, const uint8_t* payload,
                                   size_t payload_len);
};

#endif
