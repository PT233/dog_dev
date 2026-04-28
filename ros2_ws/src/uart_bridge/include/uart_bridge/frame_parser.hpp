#ifndef UART_BRIDGE_FRAME_PARSER_HPP
#define UART_BRIDGE_FRAME_PARSER_HPP

// UART 帧解析器（字节流 → 完整帧）
// 采用 8 状态状态机，逐字节处理 UART 输入流，无需缓冲区管理。
// CRC16 校验失败或尾字节错误时调用 error_callback_，并重置状态机。

#include <cstdint>
#include <cstring>
#include <vector>
#include <functional>
#include "uart_protocol.h"

class FrameParser {
public:
  // 完整帧解析成功时的回调：(cmd_id, payload 指针, payload 长度)
  using FrameCallback = std::function<void(uint8_t cmd_id, const uint8_t* payload, size_t len)>;
  // 解析错误时的回调：(错误描述字符串)
  using ErrorCallback = std::function<void(const char* msg)>;

  FrameParser();
  ~FrameParser() = default;

  void SetFrameCallback(FrameCallback cb);
  void SetErrorCallback(ErrorCallback cb);
  // 主入口：每收到一个字节调用一次，内部状态机自动推进
  void ProcessByte(uint8_t byte);

private:
  // 解析状态机状态
  // 状态转移：WAIT_HEADER_0 → WAIT_HEADER_1 → READ_CMD_ID → READ_LEN
  //        → READ_PAYLOAD（循环）→ READ_CRC_0 → READ_CRC_1 → READ_TAIL
  //        → （回调）→ WAIT_HEADER_0
  enum State {
    WAIT_HEADER_0,  // 等待帧头第 1 字节 0xAA
    WAIT_HEADER_1,  // 等待帧头第 2 字节 0x55
    READ_CMD_ID,    // 读取命令 ID
    READ_LEN,       // 读取 payload 长度
    READ_PAYLOAD,   // 逐字节读取 payload
    READ_CRC_0,     // 读取 CRC 低字节
    READ_CRC_1,     // 读取 CRC 高字节并校验
    READ_TAIL       // 读取帧尾 0x0D
  };

  State state_;
  uint8_t cmd_id_;
  uint8_t payload_len_;
  uint8_t payload_[256];  // 最大 payload 缓冲区（不含帧头/CRC/帧尾）
  uint8_t payload_idx_;   // 当前已读取的 payload 字节数
  uint8_t crc_0_;         // CRC 低字节暂存（等待高字节后合并校验）

  FrameCallback frame_callback_;
  ErrorCallback error_callback_;

  uint16_t CalculateFrameCrc() const;
  void OnFrameComplete();
  void Reset();
};

#endif
