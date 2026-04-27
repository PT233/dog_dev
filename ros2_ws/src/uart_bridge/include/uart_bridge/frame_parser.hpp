#ifndef UART_BRIDGE_FRAME_PARSER_HPP
#define UART_BRIDGE_FRAME_PARSER_HPP

#include <cstdint>
#include <cstring>
#include <vector>
#include <functional>
#include "uart_protocol.h"

class FrameParser {
public:
  using FrameCallback = std::function<void(uint8_t cmd_id, const uint8_t* payload, size_t len)>;
  using ErrorCallback = std::function<void(const char* msg)>;

  FrameParser();
  ~FrameParser() = default;

  void SetFrameCallback(FrameCallback cb);
  void SetErrorCallback(ErrorCallback cb);
  void ProcessByte(uint8_t byte);

private:
  enum State {
    WAIT_HEADER_0,
    WAIT_HEADER_1,
    READ_CMD_ID,
    READ_LEN,
    READ_PAYLOAD,
    READ_CRC_0,
    READ_CRC_1,
    READ_TAIL
  };

  State state_;
  uint8_t cmd_id_;
  uint8_t payload_len_;
  uint8_t payload_[256];
  uint8_t payload_idx_;
  uint8_t crc_0_;

  FrameCallback frame_callback_;
  ErrorCallback error_callback_;

  uint16_t CalculateFrameCrc() const;
  void OnFrameComplete();
  void Reset();
};

#endif
