#ifndef UART_BRIDGE__FRAME_PARSER_HPP_
#define UART_BRIDGE__FRAME_PARSER_HPP_

// UART 帧解析器（字节流 → 完整帧）
// 采用 8 状态状态机，逐字节处理 UART 输入流，无需缓冲区管理。
// CRC16 校验失败或尾字节错误时调用 error_callback_，并重置状态机。

#include <cstdint>
#include <cstring>
#include <vector>
#include <functional>

#include "uart_bridge/uart_protocol.h"

namespace uart_bridge
{

class FrameParser {
public:
  // 完整帧解析成功时的回调：(cmd_id, payload 指针, payload 长度)
  using FrameCallback = std::function<void(uint8_t cmd_id, const uint8_t * payload, size_t len)>;
  // 解析错误时的回调：(错误描述字符串)
  using ErrorCallback = std::function<void(const char * msg)>;

  FrameParser();
  ~FrameParser() = default;

  void set_frame_callback(FrameCallback callback);
  void set_error_callback(ErrorCallback callback);
  // 主入口：每收到一个字节调用一次，内部状态机自动推进
  void process_byte(uint8_t byte);

private:
  // 解析状态机状态
  // 状态转移：kWaitHeader0 → kWaitHeader1 → kReadCommandId → kReadLength
  //        → kReadPayload（循环）→ kReadCrcLow → kReadCrcHigh → kReadTail
  //        → （回调）→ kWaitHeader0
  enum class State
  {
    kWaitHeader0,   // 等待帧头第 1 字节 0xAA
    kWaitHeader1,   // 等待帧头第 2 字节 0x55
    kReadCommandId, // 读取命令 ID
    kReadLength,    // 读取 payload 长度
    kReadPayload,   // 逐字节读取 payload
    kReadCrcLow,    // 读取 CRC 低字节
    kReadCrcHigh,   // 读取 CRC 高字节并校验
    kReadTail       // 读取帧尾 0x0D
  };

  State state_;
  uint8_t cmd_id_;
  uint8_t payload_len_;
  uint8_t payload_[256];  // 最大 payload 缓冲区（不含帧头/CRC/帧尾）
  uint8_t payload_idx_;   // 当前已读取的 payload 字节数
  uint8_t crc_0_;         // CRC 低字节暂存（等待高字节后合并校验）

  FrameCallback frame_callback_;
  ErrorCallback error_callback_;

  uint16_t calculate_frame_crc() const;
  void handle_frame_complete();
  void reset();
};

}  // namespace uart_bridge

#endif  // UART_BRIDGE__FRAME_PARSER_HPP_
