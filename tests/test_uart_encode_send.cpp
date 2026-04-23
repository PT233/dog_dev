#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>
#include <cmath>

// Include the protocol headers
#include "../shared/uart_protocol.h"

// Mock frame encoder (simplified for testing)
class SimpleFrameEncoder {
public:
  std::vector<uint8_t> EncodeSingleServo(uint8_t servo_id, int16_t angle_x10,
                                          uint16_t duration_ms = 100) {
    ServoCmdItem item;
    item.servo_id = servo_id;
    item.angle_x10 = angle_x10;
    item.duration_ms = duration_ms;

    return BuildFrame(UART_CMD_SERVO_CONTROL, (const uint8_t*)&item,
                      sizeof(ServoCmdItem));
  }

  std::vector<uint8_t> EncodeMultipleServos(const ServoCmdItem* items,
                                             size_t count) {
    size_t payload_len = count * sizeof(ServoCmdItem);
    return BuildFrame(UART_CMD_SERVO_CONTROL, (const uint8_t*)items, payload_len);
  }

private:
  std::vector<uint8_t> BuildFrame(uint8_t cmd_id, const uint8_t* payload,
                                   size_t payload_len) {
    std::vector<uint8_t> frame;

    // Header
    frame.push_back(UART_FRAME_HEADER_0);
    frame.push_back(UART_FRAME_HEADER_1);

    // Command ID and Length
    frame.push_back(cmd_id);
    frame.push_back((uint8_t)payload_len);

    // Calculate CRC
    uint16_t crc = crc16_ccitt(&cmd_id, 1);
    crc = crc16_ccitt((const uint8_t*)&payload_len, 1);

    // Payload
    if (payload_len > 0 && payload != nullptr) {
      for (size_t i = 0; i < payload_len; ++i) {
        frame.push_back(payload[i]);
        crc = crc16_ccitt(&payload[i], 1);
      }
    }

    // CRC (big-endian)
    frame.push_back((uint8_t)((crc >> 8) & 0xFF));
    frame.push_back((uint8_t)(crc & 0xFF));

    // Tail
    frame.push_back(UART_FRAME_TAIL);

    return frame;
  }
};

// Mock parser to verify frames
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

  SimpleFrameParser() : state_(WAIT_HEADER_0), payload_len_(0) {}

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
          return true;
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

  void reset() { state_ = WAIT_HEADER_0; }
};

// Helper: Convert radians to degrees*10
int16_t RadiansToDegX10(double radians) {
  double degrees = radians * 180.0 / M_PI;
  return (int16_t)(degrees * 10.0);
}

int main() {
  std::cout << "Testing UART Frame Encoding and Sending\n\n";

  // Test 1: Encode single servo command (yaw = 90 degrees = pi/2 radians)
  {
    std::cout << "Test 1: Encode single servo command (yaw = 90°)\n";

    SimpleFrameEncoder encoder;
    int16_t angle_x10 = RadiansToDegX10(M_PI / 2.0);  // 90 degrees
    auto frame = encoder.EncodeSingleServo(0, angle_x10, 100);

    // Verify frame structure
    assert(frame.size() >= 8 && "Frame too short");
    assert(frame[0] == UART_FRAME_HEADER_0 && "Invalid header 0");
    assert(frame[1] == UART_FRAME_HEADER_1 && "Invalid header 1");
    assert(frame[2] == UART_CMD_SERVO_CONTROL && "Invalid command ID");
    assert(frame.back() == UART_FRAME_TAIL && "Invalid tail");

    // Parse the frame back to verify it's correct
    SimpleFrameParser parser;
    bool complete = false;
    for (uint8_t byte : frame) {
      if (parser.ProcessByte(byte)) {
        complete = true;
        break;
      }
    }

    assert(complete && "Frame should parse successfully");
    assert(parser.GetCmdId() == UART_CMD_SERVO_CONTROL && "Command ID mismatch");
    assert(parser.GetPayloadLen() == sizeof(ServoCmdItem) && "Payload length mismatch");

    const ServoCmdItem* parsed = (const ServoCmdItem*)parser.GetPayload();
    assert(parsed->servo_id == 0 && "Servo ID mismatch");
    assert(parsed->angle_x10 == 900 && "Angle mismatch");
    assert(parsed->duration_ms == 100 && "Duration mismatch");

    std::cout << "  ✓ Successfully encoded and verified: servo_id=0, angle=90°, duration=100ms\n\n";
  }

  // Test 2: Encode multiple servo commands
  {
    std::cout << "Test 2: Encode multiple servo commands\n";

    SimpleFrameEncoder encoder;
    ServoCmdItem items[2];

    // Servo 0 (yaw): 45 degrees
    items[0].servo_id = 0;
    items[0].angle_x10 = RadiansToDegX10(M_PI / 4.0);  // 45 degrees
    items[0].duration_ms = 500;

    // Servo 1 (pitch): 90 degrees
    items[1].servo_id = 1;
    items[1].angle_x10 = RadiansToDegX10(M_PI / 2.0);  // 90 degrees
    items[1].duration_ms = 500;

    auto frame = encoder.EncodeMultipleServos(items, 2);

    // Parse frame
    SimpleFrameParser parser;
    bool complete = false;
    for (uint8_t byte : frame) {
      if (parser.ProcessByte(byte)) {
        complete = true;
        break;
      }
    }

    assert(complete && "Frame should parse successfully");
    assert(parser.GetPayloadLen() == sizeof(ServoCmdItem) * 2 && "Payload length mismatch");

    const ServoCmdItem* parsed = (const ServoCmdItem*)parser.GetPayload();
    assert(parsed[0].servo_id == 0 && "First servo ID mismatch");
    assert(parsed[0].angle_x10 == 450 && "First servo angle mismatch");
    assert(parsed[1].servo_id == 1 && "Second servo ID mismatch");
    assert(parsed[1].angle_x10 == 900 && "Second servo angle mismatch");

    std::cout << "  ✓ Successfully encoded multiple: servo0=45°, servo1=90°\n\n";
  }

  // Test 3: Test pitch servo (negative and positive angles)
  {
    std::cout << "Test 3: Test pitch servo with various angles\n";

    SimpleFrameEncoder encoder;

    // Test angles: 0°, 45°, 90°, 135°, 180°
    int angles_deg[] = {0, 45, 90, 135, 180};

    for (int i = 0; i < 5; ++i) {
      double radians = angles_deg[i] * M_PI / 180.0;
      int16_t angle_x10 = RadiansToDegX10(radians);
      auto frame = encoder.EncodeSingleServo(1, angle_x10, 250);

      SimpleFrameParser parser;
      bool complete = false;
      for (uint8_t byte : frame) {
        if (parser.ProcessByte(byte)) {
          complete = true;
          break;
        }
      }

      assert(complete && "Frame should parse");
      const ServoCmdItem* parsed = (const ServoCmdItem*)parser.GetPayload();
      // Allow ±1 tolerance for floating point precision
      int expected = angles_deg[i] * 10;
      assert(parsed->angle_x10 >= expected - 1 && parsed->angle_x10 <= expected + 1 &&
             "Angle conversion mismatch");
    }

    std::cout << "  ✓ Successfully verified angle conversions: 0°, 45°, 90°, 135°, 180°\n\n";
  }

  // Test 4: Servo ID mapping
  {
    std::cout << "Test 4: Test servo ID mapping\n";

    SimpleFrameEncoder encoder;
    uint8_t servo_ids[] = {0, 1, 2, 3};

    for (int i = 0; i < 4; ++i) {
      auto frame = encoder.EncodeSingleServo(servo_ids[i], 0, 100);

      SimpleFrameParser parser;
      bool complete = false;
      for (uint8_t byte : frame) {
        if (parser.ProcessByte(byte)) {
          complete = true;
          break;
        }
      }

      assert(complete && "Frame should parse");
      const ServoCmdItem* parsed = (const ServoCmdItem*)parser.GetPayload();
      assert(parsed->servo_id == servo_ids[i] && "Servo ID mismatch");
    }

    std::cout << "  ✓ Successfully verified servo ID mapping: 0-3\n\n";
  }

  std::cout << "All encoding tests passed! ✓\n";
  return 0;
}
