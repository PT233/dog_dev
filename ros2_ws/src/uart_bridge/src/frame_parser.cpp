#include "uart_bridge/frame_parser.hpp"

FrameParser::FrameParser()
    : state_(WAIT_HEADER_0),
      cmd_id_(0),
      payload_len_(0),
      payload_idx_(0),
      crc_0_(0) {}

void FrameParser::SetFrameCallback(FrameCallback cb) {
  frame_callback_ = cb;
}

void FrameParser::SetErrorCallback(ErrorCallback cb) {
  error_callback_ = cb;
}

void FrameParser::ProcessByte(uint8_t byte) {
  switch (state_) {
    case WAIT_HEADER_0:
      if (byte == UART_FRAME_HEADER_0) {
        state_ = WAIT_HEADER_1;
      }
      break;

    case WAIT_HEADER_1:
      if (byte == UART_FRAME_HEADER_1) {
        state_ = READ_CMD_ID;
      } else if (byte == UART_FRAME_HEADER_0) {
        state_ = WAIT_HEADER_1;
      } else {
        state_ = WAIT_HEADER_0;
      }
      break;

    case READ_CMD_ID:
      cmd_id_ = byte;
      state_ = READ_LEN;
      break;

    case READ_LEN:
      payload_len_ = byte;
      // 最大合法 payload = 256 - 7（帧头2 + CMD_ID1 + LEN1 + CRC2 + 尾1）
      if (payload_len_ > UART_MAX_FRAME_LEN - 7) {
        if (error_callback_) {
          error_callback_("Payload length too large");
        }
        Reset();
      } else {
        payload_idx_ = 0;
        // payload 长度为 0 时直接跳到 CRC 阶段（如 QUERY 帧）
        state_ = (payload_len_ == 0) ? READ_CRC_0 : READ_PAYLOAD;
      }
      break;

    case READ_PAYLOAD:
      payload_[payload_idx_++] = byte;
      if (payload_idx_ >= payload_len_) {
        state_ = READ_CRC_0;
      }
      break;

    case READ_CRC_0:
      crc_0_ = byte;
      state_ = READ_CRC_1;
      break;

    case READ_CRC_1: {
      // 帧中 CRC 以小端序传输：低字节(crc_0_)先到，高字节后到
      uint16_t crc_received = ((uint16_t)byte << 8) | crc_0_;
      uint16_t crc_calculated = CalculateFrameCrc();
      if (crc_received != crc_calculated) {
        if (error_callback_) {
          error_callback_("CRC mismatch");
        }
        Reset();
      } else {
        state_ = READ_TAIL;
      }
      break;
    }

    case READ_TAIL:
      if (byte == UART_FRAME_TAIL) {
        OnFrameComplete();
      } else {
        if (error_callback_) {
          error_callback_("Invalid frame tail");
        }
      }
      Reset();
      break;
  }
}

uint16_t FrameParser::CalculateFrameCrc() const {
  uint8_t crc_data[UART_MAX_FRAME_LEN - 5];
  crc_data[0] = cmd_id_;
  crc_data[1] = payload_len_;
  if (payload_len_ > 0) {
    memcpy(&crc_data[2], payload_, payload_len_);
  }
  return crc16_ccitt(crc_data, (size_t)payload_len_ + 2);
}

void FrameParser::OnFrameComplete() {
  if (frame_callback_) {
    frame_callback_(cmd_id_, payload_, payload_len_);
  }
}

void FrameParser::Reset() {
  state_ = WAIT_HEADER_0;
  payload_idx_ = 0;
}
