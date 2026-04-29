#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>

// Include the protocol headers
#include "../shared/uart_protocol.h"

// Mock frame parser (simplified version for testing)
class SimpleFrameParser {
public:
  enum State {
    kWaitHeader0,
    kWaitHeader1,
    kReadCmdId,
    kReadLength,
    kReadPayload,
    kReadCrc0,
    kReadCrc1,
    kReadTail
  };

  SimpleFrameParser() : state_(kWaitHeader0), payload_len_(0), payload_idx_(0) {}

  bool process_byte(uint8_t byte) {
    switch (state_) {
      case kWaitHeader0:
        if (byte == UART_FRAME_HEADER_0) {
          state_ = kWaitHeader1;
        }
        break;

      case kWaitHeader1:
        if (byte == UART_FRAME_HEADER_1) {
          state_ = kReadCmdId;
        } else {
          state_ = kWaitHeader0;
        }
        break;

      case kReadCmdId:
        cmd_id_ = byte;
        state_ = kReadLength;
        break;

      case kReadLength:
        payload_len_ = byte;
        payload_idx_ = 0;
        state_ = (payload_len_ == 0) ? kReadCrc0 : kReadPayload;
        break;

      case kReadPayload:
        payload_[payload_idx_++] = byte;
        if (payload_idx_ >= payload_len_) {
          state_ = kReadCrc0;
        }
        break;

      case kReadCrc0:
        crc_0_ = byte;
        state_ = kReadCrc1;
        break;

      case kReadCrc1: {
        uint16_t crc_received = ((uint16_t)byte << 8) | crc_0_;
        if (crc_received != calculate_crc()) {
          reset();
          return false;
        }
        state_ = kReadTail;
        break;
      }

      case kReadTail:
        if (byte == UART_FRAME_TAIL) {
          reset();
          return true;  // Frame complete
        }
        reset();
        return false;
    }
    return false;
  }

  uint8_t cmd_id() const { return cmd_id_; }
  const uint8_t* payload() const { return payload_; }
  size_t payload_length() const { return payload_len_; }

private:
  State state_;
  uint8_t cmd_id_;
  uint8_t payload_len_;
  uint8_t payload_[256];
  uint8_t payload_idx_;
  uint8_t crc_0_;

  uint16_t calculate_crc() const {
    uint8_t crc_data[UART_MAX_FRAME_LEN];
    crc_data[0] = cmd_id_;
    crc_data[1] = payload_len_;
    if (payload_len_ > 0) {
      std::memcpy(&crc_data[2], payload_, payload_len_);
    }
    return crc16_ccitt(crc_data, payload_len_ + 2);
  }

  void reset() { state_ = kWaitHeader0; payload_idx_ = 0; }
};

// Helper to build a complete frame
std::vector<uint8_t> build_frame(uint8_t cmd_id, const uint8_t* payload, size_t payload_len) {
  std::vector<uint8_t> frame;
  frame.push_back(UART_FRAME_HEADER_0);
  frame.push_back(UART_FRAME_HEADER_1);
  frame.push_back(cmd_id);
  frame.push_back((uint8_t)payload_len);

  if (payload_len > 0) {
    for (size_t i = 0; i < payload_len; ++i) {
      frame.push_back(payload[i]);
    }
  }

  uint16_t crc = crc16_ccitt(&frame[2], payload_len + 2);
  frame.push_back((uint8_t)(crc & 0xFF));
  frame.push_back((uint8_t)((crc >> 8) & 0xFF));
  frame.push_back(UART_FRAME_TAIL);

  return frame;
}

int main() {
  std::cout << "Testing UART Frame Parser\n\n";

  // Test 1: Parse servo state frame
  {
    std::cout << "Test 1: Parse servo state frame (0x81)\n";
    ServoStateItem state;
    state.servo_id = 0;
    state.current_angle_x10 = 900;  // 90.0 degrees
    state.status = 0;

    auto frame = build_frame(kUartCmdServoState, (const uint8_t*)&state, sizeof(state));
    SimpleFrameParser parser;

    bool complete = false;
    for (uint8_t byte : frame) {
      if (parser.process_byte(byte)) {
        complete = true;
        break;
      }
    }

    assert(complete && "Frame should be complete");
    assert(parser.cmd_id() == kUartCmdServoState && "Command ID should match");
    assert(parser.payload_length() == sizeof(ServoStateItem) && "Payload length should match");

    const ServoStateItem* parsed = (const ServoStateItem*)parser.payload();
    assert(parsed->servo_id == 0 && "Servo ID should be 0");
    assert(parsed->current_angle_x10 == 900 && "Angle should be 900 (90.0°)");
    assert(parsed->status == 0 && "Status should be 0");

    std::cout << "  ✓ Successfully parsed servo state: id=" << (int)parsed->servo_id
              << " angle=" << (parsed->current_angle_x10 / 10.0) << "° status=" << (int)parsed->status << "\n\n";
  }

  // Test 2: Parse servo control frame
  {
    std::cout << "Test 2: Parse servo control frame (0x01)\n";
    ServoCmdItem cmd;
    cmd.servo_id = 1;
    cmd.angle_x10 = 1800;  // 180.0 degrees
    cmd.duration_ms = 1000;

    auto frame = build_frame(kUartCmdServoControl, (const uint8_t*)&cmd, sizeof(cmd));
    SimpleFrameParser parser;

    bool complete = false;
    for (uint8_t byte : frame) {
      if (parser.process_byte(byte)) {
        complete = true;
        break;
      }
    }

    assert(complete && "Frame should be complete");
    assert(parser.cmd_id() == kUartCmdServoControl && "Command ID should match");

    const ServoCmdItem* parsed = (const ServoCmdItem*)parser.payload();
    assert(parsed->servo_id == 1 && "Servo ID should be 1");
    assert(parsed->angle_x10 == 1800 && "Angle should be 1800 (180.0°)");
    assert(parsed->duration_ms == 1000 && "Duration should be 1000ms");

    std::cout << "  ✓ Successfully parsed servo control: id=" << (int)parsed->servo_id
              << " angle=" << (parsed->angle_x10 / 10.0) << "° duration=" << parsed->duration_ms << "ms\n\n";
  }

  // Test 3: Handle corrupted frame (bad CRC)
  {
    std::cout << "Test 3: Handle corrupted frame (bad CRC)\n";
    ServoStateItem state;
    state.servo_id = 0;
    state.current_angle_x10 = 500;
    state.status = 1;

    auto frame = build_frame(kUartCmdServoState, (const uint8_t*)&state, sizeof(state));
    // Corrupt the CRC
    if (frame.size() > 3) {
      frame[frame.size() - 3] ^= 0xFF;  // Flip bits in first CRC byte
    }

    SimpleFrameParser parser;
    bool complete = false;
    for (uint8_t byte : frame) {
      if (parser.process_byte(byte)) {
        complete = true;
        break;
      }
    }

    assert(!complete && "Corrupted frame should not complete");
    std::cout << "  ✓ Correctly rejected corrupted frame\n\n";
  }

  // Test 4: Recover from garbage bytes
  {
    std::cout << "Test 4: Recover from garbage bytes\n";
    ServoStateItem state;
    state.servo_id = 2;
    state.current_angle_x10 = 1350;
    state.status = 0;

    auto frame = build_frame(kUartCmdServoState, (const uint8_t*)&state, sizeof(state));

    // Insert garbage before valid frame
    std::vector<uint8_t> data;
    data.push_back(0xFF);
    data.push_back(0xEE);
    data.insert(data.end(), frame.begin(), frame.end());

    SimpleFrameParser parser;
    bool complete = false;
    for (uint8_t byte : data) {
      if (parser.process_byte(byte)) {
        complete = true;
        break;
      }
    }

    assert(complete && "Should recover from garbage bytes");
    assert(parser.cmd_id() == kUartCmdServoState && "Command ID should match");

    const ServoStateItem* parsed = (const ServoStateItem*)parser.payload();
    assert(parsed->servo_id == 2 && "Servo ID should be 2");

    std::cout << "  ✓ Successfully recovered from garbage bytes\n\n";
  }

  std::cout << "All tests passed! ✓\n";
  return 0;
}
