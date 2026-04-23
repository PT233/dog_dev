#include "uart_bridge/frame_parser.hpp"

FrameParser::FrameParser()
    : state_(WAIT_HEADER_0),
      cmd_id_(0),
      payload_len_(0),
      payload_idx_(0),
      crc_0_(0),
      crc_calculated_(0) {}

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
      if (payload_len_ > 250) {  // Sanity check
        if (error_callback_) {
          error_callback_("Payload length too large");
        }
        Reset();
      } else {
        payload_idx_ = 0;
        crc_calculated_ = crc16_ccitt(&cmd_id_, 1);
        crc_calculated_ = crc16_ccitt(&payload_len_, 1);
        state_ = (payload_len_ == 0) ? READ_CRC_0 : READ_PAYLOAD;
      }
      break;

    case READ_PAYLOAD:
      payload_[payload_idx_++] = byte;
      crc_calculated_ = crc16_ccitt(&byte, 1);
      if (payload_idx_ >= payload_len_) {
        state_ = READ_CRC_0;
      }
      break;

    case READ_CRC_0:
      crc_0_ = byte;
      state_ = READ_CRC_1;
      break;

    case READ_CRC_1: {
      uint16_t crc_received = ((uint16_t)crc_0_ << 8) | byte;
      if (crc_received != crc_calculated_) {
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
        OnFrameComplete(crc_0_, byte);
      } else {
        if (error_callback_) {
          error_callback_("Invalid frame tail");
        }
      }
      Reset();
      break;
  }
}

void FrameParser::OnFrameComplete(uint8_t, uint8_t) {
  if (frame_callback_) {
    frame_callback_(cmd_id_, payload_, payload_len_);
  }
}

void FrameParser::Reset() {
  state_ = WAIT_HEADER_0;
  payload_idx_ = 0;
}
