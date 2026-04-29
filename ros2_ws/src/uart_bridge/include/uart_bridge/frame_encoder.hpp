#ifndef UART_BRIDGE__FRAME_ENCODER_HPP_
#define UART_BRIDGE__FRAME_ENCODER_HPP_

// UART 帧编码器（结构化数据 → 字节流）
// 负责将 ROS 2 消息数据序列化为符合协议规范的 UART 二进制帧，
// 包括帧头、CMD_ID、payload 和 CRC16 尾部。

#include <cstdint>
#include <vector>

#include "uart_bridge/uart_protocol.h"

namespace uart_bridge
{

class FrameEncoder {
public:
  FrameEncoder();
  ~FrameEncoder() = default;

  // 将 N 路舵机指令编码为 CMD_ID=0x01 的 SERVO_CONTROL 帧
  // payload = ServoCmdItem × count
  std::vector<uint8_t> encode_servo_control(const ServoCmdItem * items, size_t count);

  // 便捷接口：单路舵机指令编码（内部调用 encode_servo_control）
  std::vector<uint8_t> encode_single_servo(
    uint8_t servo_id, int16_t angle_x10,
    uint16_t duration_ms = 100);

  // 编码握手帧（CMD_ID=0x10），发送协议版本号和请求状态
  std::vector<uint8_t> encode_init_handshake();

private:
  // 底层帧构建：填充帧头、CMD_ID、LEN、payload，计算并附加 CRC16，添加帧尾
  std::vector<uint8_t> build_frame(
    uint8_t cmd_id, const uint8_t * payload,
    size_t payload_len);
};

}  // namespace uart_bridge

#endif  // UART_BRIDGE__FRAME_ENCODER_HPP_
