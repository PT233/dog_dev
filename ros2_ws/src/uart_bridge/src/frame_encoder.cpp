#include "uart_bridge/frame_encoder.hpp"
#include <cstring>

FrameEncoder::FrameEncoder() {}

std::vector<uint8_t> FrameEncoder::EncodeServoControl(const ServoCmdItem* items,
                                                       size_t count) {
  if (count == 0 || items == nullptr) {
    return {};
  }

  // ServoCmdItem 已是 packed 协议结构，可直接作为 payload 字节序列发送。
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

std::vector<uint8_t> FrameEncoder::EncodeInitHandshake() {
  UartHandshakePayload payload;
  // 握手 payload 声明协议版本并请求 STM32 进入 ACTIVE；STM32 会按状态机决定是否接受。
  payload.protocol_version = UART_PROTOCOL_VERSION;
  payload.requested_state = UART_SYSTEM_STATE_ACTIVE;
  payload.reserved = 0;

  return BuildFrame(UART_CMD_INIT_HANDSHAKE,
                    reinterpret_cast<const uint8_t*>(&payload),
                    sizeof(payload));
}

std::vector<uint8_t> FrameEncoder::BuildFrame(uint8_t cmd_id,
                                               const uint8_t* payload,
                                               size_t payload_len) {
  std::vector<uint8_t> frame;
  if (payload_len > UART_MAX_FRAME_LEN - 7) {
    // 7 字节固定开销：帧头2 + CMD1 + LEN1 + CRC2 + 帧尾1。
    return {};
  }

  // 帧头
  frame.push_back(UART_FRAME_HEADER_0);
  frame.push_back(UART_FRAME_HEADER_1);

  // 命令 ID 和 payload 长度
  frame.push_back(cmd_id);
  frame.push_back((uint8_t)payload_len);

  // payload 数据
  if (payload_len > 0 && payload != nullptr) {
    for (size_t i = 0; i < payload_len; ++i) {
      frame.push_back(payload[i]);
    }
  }

  // CRC16：覆盖 CMD_ID + LEN + PAYLOAD（即 frame[2] 起），小端序发送
  // 与 STM32 端 FrameParser::CalculateFrameCrc() 计算范围一致
  uint16_t crc = crc16_ccitt(&frame[2], payload_len + 2);
  frame.push_back((uint8_t)(crc & 0xFF));        // 低字节先发
  frame.push_back((uint8_t)((crc >> 8) & 0xFF)); // 高字节后发

  // 帧尾
  frame.push_back(UART_FRAME_TAIL);

  return frame;
}
