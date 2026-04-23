#include "uart_bridge/frame_encoder.hpp"
#include <cstring>

FrameEncoder::FrameEncoder() {}

std::vector<uint8_t> FrameEncoder::EncodeServoControl(const ServoCmdItem* items,
                                                       size_t count) {
  if (count == 0 || items == nullptr) {
    return {};
  }

  size_t payload_len = count * sizeof(ServoCmdItem);
  const uint8_t* payload = reinterpret_cast<const uint8_t*>(items);

  return BuildFrame(UART_CMD_SERVO_CONTROL, payload, payload_len);
}

std::vector<uint8_t> FrameEncoder::EncodeSingleServo(uint8_t servo_id,
                                                      int16_t angle_x10,
                                                      uint16_t duration_ms) {
  ServoCmdItem item;
  item.servo_id = servo_id;
  item.angle_x10 = angle_x10;
  item.duration_ms = duration_ms;

  return EncodeServoControl(&item, 1);
}

std::vector<uint8_t> FrameEncoder::BuildFrame(uint8_t cmd_id,
                                               const uint8_t* payload,
                                               size_t payload_len) {
  std::vector<uint8_t> frame;

  // Header
  frame.push_back(UART_FRAME_HEADER_0);
  frame.push_back(UART_FRAME_HEADER_1);

  // Command ID and Length
  frame.push_back(cmd_id);
  frame.push_back((uint8_t)payload_len);

  // Calculate CRC over CMD_ID + LEN + PAYLOAD
  uint16_t crc = crc16_ccitt(&cmd_id, 1);
  crc = crc16_ccitt((const uint8_t*)&payload_len, 1);

  // Payload
  if (payload_len > 0 && payload != nullptr) {
    for (size_t i = 0; i < payload_len; ++i) {
      frame.push_back(payload[i]);
      crc = crc16_ccitt(&payload[i], 1);
    }
  }

  // CRC (big-endian)
  frame.push_back((uint8_t)((crc >> 8) & 0xFF));
  frame.push_back((uint8_t)(crc & 0xFF));

  // Tail
  frame.push_back(UART_FRAME_TAIL);

  return frame;
}
