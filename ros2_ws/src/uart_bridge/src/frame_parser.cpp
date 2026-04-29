#include "uart_bridge/frame_parser.hpp"

namespace uart_bridge
{

FrameParser::FrameParser()
: state_(State::kWaitHeader0),
  cmd_id_(0),
  payload_len_(0),
  payload_idx_(0),
  crc_0_(0) {}

void FrameParser::set_frame_callback(FrameCallback callback)
{
  frame_callback_ = callback;
}

void FrameParser::set_error_callback(ErrorCallback callback)
{
  error_callback_ = callback;
}

void FrameParser::process_byte(uint8_t byte)
{
  switch (state_) {
    case State::kWaitHeader0:
      // 未同步状态只寻找 0xAA，丢弃所有其他噪声字节。
      if (byte == UART_FRAME_HEADER_0) {
        state_ = State::kWaitHeader1;
      }
      break;

    case State::kWaitHeader1:
      if (byte == UART_FRAME_HEADER_1) {
        state_ = State::kReadCommandId;
      } else if (byte == UART_FRAME_HEADER_0) {
        // 连续 0xAA 时保留第二个作为潜在新帧头，提高重同步速度。
        state_ = State::kWaitHeader1;
      } else {
        state_ = State::kWaitHeader0;
      }
      break;

    case State::kReadCommandId:
      cmd_id_ = byte;
      state_ = State::kReadLength;
      break;

    case State::kReadLength:
      payload_len_ = byte;
      // 最大合法 payload = 256 - 7（帧头2 + CMD_ID1 + LEN1 + CRC2 + 尾1）
      if (payload_len_ > UART_MAX_FRAME_LEN - 7) {
        if (error_callback_) {
          error_callback_("Payload length too large");
        }
        reset();
      } else {
        payload_idx_ = 0;
        // payload 长度为 0 时直接跳到 CRC 阶段（如 QUERY 帧）
        state_ = (payload_len_ == 0) ? State::kReadCrcLow : State::kReadPayload;
      }
      break;

    case State::kReadPayload:
      payload_[payload_idx_++] = byte;
      if (payload_idx_ >= payload_len_) {
        state_ = State::kReadCrcLow;
      }
      break;

    case State::kReadCrcLow:
      crc_0_ = byte;
      state_ = State::kReadCrcHigh;
      break;

    case State::kReadCrcHigh: {
      // 帧中 CRC 以小端序传输：低字节(crc_0_)先到，高字节后到
        uint16_t crc_received = ((uint16_t)byte << 8) | crc_0_;
        uint16_t crc_calculated = calculate_frame_crc();
        if (crc_received != crc_calculated) {
          if (error_callback_) {
            error_callback_("CRC mismatch");
          }
          reset();
        } else {
          state_ = State::kReadTail;
        }
        break;
      }

    case State::kReadTail:
      if (byte == UART_FRAME_TAIL) {
        handle_frame_complete();
      } else {
        if (error_callback_) {
          error_callback_("Invalid frame tail");
        }
      }
      reset();
      break;
  }
}

uint16_t FrameParser::calculate_frame_crc() const
{
  uint8_t crc_data[UART_MAX_FRAME_LEN - 5];
  // CRC 覆盖范围与 STM32 端完全一致：CMD_ID + LEN + PAYLOAD。
  crc_data[0] = cmd_id_;
  crc_data[1] = payload_len_;
  if (payload_len_ > 0) {
    memcpy(&crc_data[2], payload_, payload_len_);
  }
  return crc16_ccitt(crc_data, (size_t)payload_len_ + 2);
}

void FrameParser::handle_frame_complete()
{
  if (frame_callback_) {
    frame_callback_(cmd_id_, payload_, payload_len_);
  }
}

void FrameParser::reset()
{
  state_ = State::kWaitHeader0;
  payload_idx_ = 0;
}

}  // namespace uart_bridge
