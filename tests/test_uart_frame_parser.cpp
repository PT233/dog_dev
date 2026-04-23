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
    WAIT_HEADER_0,
    WAIT_HEADER_1,
    READ_CMD_ID,
    READ_LEN,
    READ_PAYLOAD,
    READ_CRC_0,
    READ_CRC_1,
    READ_TAIL
  };

  SimpleFrameParser() : state_(WAIT_HEADER_0), payload_len_(0), payload_idx_(0) {}

  bool ProcessByte(uint8_t byte) {
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
        payload_idx_ = 0;
        crc_calculated_ = crc16_ccitt(&cmd_id_, 1);
        crc_calculated_ = crc16_ccitt(&payload_len_, 1);
        state_ = (payload_len_ == 0) ? READ_CRC_0 : READ_PAYLOAD;
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
          reset();
          return false;
        }
        state_ = READ_TAIL;
        break;
      }

      case READ_TAIL:
        if (byte == UART_FRAME_TAIL) {
          reset();
          return true;  // Frame complete
        }
        reset();
        return false;
    }
    return false;
  }

  uint8_t GetCmdId() const { return cmd_id_; }
  const uint8_t* GetPayload() const { return payload_; }
  size_t GetPayloadLen() const { return payload_len_; }

private:
  State state_;
  uint8_t cmd_id_;
  uint8_t payload_len_;
  uint8_t payload_[256];
  uint8_t payload_idx_;
  uint8_t crc_0_;
  uint16_t crc_calculated_;

  void reset() { state_ = WAIT_HEADER_0; payload_idx_ = 0; }
};

// Helper to build a complete frame
std::vector<uint8_t> BuildFrame(uint8_t cmd_id, const uint8_t* payload, size_t payload_len) {
  std::vector<uint8_t> frame;
  frame.push_back(UART_FRAME_HEADER_0);
  frame.push_back(UART_FRAME_HEADER_1);
  frame.push_back(cmd_id);
  frame.push_back((uint8_t)payload_len);

  uint16_t crc = crc16_ccitt(&cmd_id, 1);
  crc = crc16_ccitt((const uint8_t*)&payload_len, 1);

  if (payload_len > 0) {
    for (size_t i = 0; i < payload_len; ++i) {
      frame.push_back(payload[i]);
      crc = crc16_ccitt(&payload[i], 1);
    }
  }

  frame.push_back((uint8_t)((crc >> 8) & 0xFF));
  frame.push_back((uint8_t)(crc & 0xFF));
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

    auto frame = BuildFrame(UART_CMD_SERVO_STATE, (const uint8_t*)&state, sizeof(state));
    SimpleFrameParser parser;

    bool complete = false;
    for (uint8_t byte : frame) {
      if (parser.ProcessByte(byte)) {
        complete = true;
        break;
      }
    }

    assert(complete && "Frame should be complete");
    assert(parser.GetCmdId() == UART_CMD_SERVO_STATE && "Command ID should match");
    assert(parser.GetPayloadLen() == sizeof(ServoStateItem) && "Payload length should match");

    const ServoStateItem* parsed = (const ServoStateItem*)parser.GetPayload();
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

    auto frame = BuildFrame(UART_CMD_SERVO_CONTROL, (const uint8_t*)&cmd, sizeof(cmd));
    SimpleFrameParser parser;

    bool complete = false;
    for (uint8_t byte : frame) {
      if (parser.ProcessByte(byte)) {
        complete = true;
        break;
      }
    }

    assert(complete && "Frame should be complete");
    assert(parser.GetCmdId() == UART_CMD_SERVO_CONTROL && "Command ID should match");

    const ServoCmdItem* parsed = (const ServoCmdItem*)parser.GetPayload();
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

    auto frame = BuildFrame(UART_CMD_SERVO_STATE, (const uint8_t*)&state, sizeof(state));
    // Corrupt the CRC
    if (frame.size() > 3) {
      frame[frame.size() - 3] ^= 0xFF;  // Flip bits in first CRC byte
    }

    SimpleFrameParser parser;
    bool complete = false;
    for (uint8_t byte : frame) {
      if (parser.ProcessByte(byte)) {
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

    auto frame = BuildFrame(UART_CMD_SERVO_STATE, (const uint8_t*)&state, sizeof(state));

    // Insert garbage before valid frame
    std::vector<uint8_t> data;
    data.push_back(0xFF);
    data.push_back(0xEE);
    data.insert(data.end(), frame.begin(), frame.end());

    SimpleFrameParser parser;
    bool complete = false;
    for (uint8_t byte : data) {
      if (parser.ProcessByte(byte)) {
        complete = true;
        break;
      }
    }

    assert(complete && "Should recover from garbage bytes");
    assert(parser.GetCmdId() == UART_CMD_SERVO_STATE && "Command ID should match");

    const ServoStateItem* parsed = (const ServoStateItem*)parser.GetPayload();
    assert(parsed->servo_id == 2 && "Servo ID should be 2");

    std::cout << "  ✓ Successfully recovered from garbage bytes\n\n";
  }

  std::cout << "All tests passed! ✓\n";
  return 0;
}
