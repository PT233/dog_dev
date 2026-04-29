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
  std::vector<uint8_t> encode_single_servo(uint8_t servo_id, int16_t angle_x10,
                                          uint16_t duration_ms = 100) {
    ServoCmdItem item;
    item.servo_id = servo_id;
    item.angle_x10 = angle_x10;
    item.duration_ms = duration_ms;

    return build_frame(kUartCmdServoControl, (const uint8_t*)&item,
                      sizeof(ServoCmdItem));
  }

  std::vector<uint8_t> encode_multiple_servos(const ServoCmdItem* items,
                                             size_t count) {
    size_t payload_len = count * sizeof(ServoCmdItem);
    return build_frame(kUartCmdServoControl, (const uint8_t*)items, payload_len);
  }

private:
  std::vector<uint8_t> build_frame(uint8_t cmd_id, const uint8_t* payload,
                                   size_t payload_len) {
    std::vector<uint8_t> frame;

    // Header
    frame.push_back(UART_FRAME_HEADER_0);
    frame.push_back(UART_FRAME_HEADER_1);

    // Command ID and Length
    frame.push_back(cmd_id);
    frame.push_back((uint8_t)payload_len);

    // Payload
    if (payload_len > 0 && payload != nullptr) {
      for (size_t i = 0; i < payload_len; ++i) {
        frame.push_back(payload[i]);
      }
    }

    // CRC over CMD_ID + LEN + PAYLOAD, little-endian on the wire.
    uint16_t crc = crc16_ccitt(&frame[2], payload_len + 2);
    frame.push_back((uint8_t)(crc & 0xFF));
    frame.push_back((uint8_t)((crc >> 8) & 0xFF));

    // Tail
    frame.push_back(UART_FRAME_TAIL);

    return frame;
  }
};

// Mock parser to verify frames
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

  SimpleFrameParser() : state_(kWaitHeader0), payload_len_(0) {}

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
          return true;
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

  void reset() { state_ = kWaitHeader0; }
};

// Helper: Convert radians to degrees*10
int16_t radians_to_deg_x10(double radians) {
  double degrees = radians * 180.0 / M_PI;
  return (int16_t)(degrees * 10.0);
}

int main() {
  std::cout << "Testing UART Frame Encoding and Sending\n\n";

  // Test 1: Encode single servo command (yaw = 90 degrees = pi/2 radians)
  {
    std::cout << "Test 1: Encode single servo command (yaw = 90°)\n";

    SimpleFrameEncoder encoder;
    int16_t angle_x10 = radians_to_deg_x10(M_PI / 2.0);  // 90 degrees
    auto frame = encoder.encode_single_servo(0, angle_x10, 100);

    // Verify frame structure
    assert(frame.size() >= 8 && "Frame too short");
    assert(frame[0] == UART_FRAME_HEADER_0 && "Invalid header 0");
    assert(frame[1] == UART_FRAME_HEADER_1 && "Invalid header 1");
    assert(frame[2] == kUartCmdServoControl && "Invalid command ID");
    assert(frame.back() == UART_FRAME_TAIL && "Invalid tail");

    // Parse the frame back to verify it's correct
    SimpleFrameParser parser;
    bool complete = false;
    for (uint8_t byte : frame) {
      if (parser.process_byte(byte)) {
        complete = true;
        break;
      }
    }

    assert(complete && "Frame should parse successfully");
    assert(parser.cmd_id() == kUartCmdServoControl && "Command ID mismatch");
    assert(parser.payload_length() == sizeof(ServoCmdItem) && "Payload length mismatch");

    const ServoCmdItem* parsed = (const ServoCmdItem*)parser.payload();
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
    items[0].angle_x10 = radians_to_deg_x10(M_PI / 4.0);  // 45 degrees
    items[0].duration_ms = 500;

    // Servo 1 (pitch): 90 degrees
    items[1].servo_id = 1;
    items[1].angle_x10 = radians_to_deg_x10(M_PI / 2.0);  // 90 degrees
    items[1].duration_ms = 500;

    auto frame = encoder.encode_multiple_servos(items, 2);

    // Parse frame
    SimpleFrameParser parser;
    bool complete = false;
    for (uint8_t byte : frame) {
      if (parser.process_byte(byte)) {
        complete = true;
        break;
      }
    }

    assert(complete && "Frame should parse successfully");
    assert(parser.payload_length() == sizeof(ServoCmdItem) * 2 && "Payload length mismatch");

    const ServoCmdItem* parsed = (const ServoCmdItem*)parser.payload();
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
      int16_t angle_x10 = radians_to_deg_x10(radians);
      auto frame = encoder.encode_single_servo(1, angle_x10, 250);

      SimpleFrameParser parser;
      bool complete = false;
      for (uint8_t byte : frame) {
        if (parser.process_byte(byte)) {
          complete = true;
          break;
        }
      }

      assert(complete && "Frame should parse");
      const ServoCmdItem* parsed = (const ServoCmdItem*)parser.payload();
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
      auto frame = encoder.encode_single_servo(servo_ids[i], 0, 100);

      SimpleFrameParser parser;
      bool complete = false;
      for (uint8_t byte : frame) {
        if (parser.process_byte(byte)) {
          complete = true;
          break;
        }
      }

      assert(complete && "Frame should parse");
      const ServoCmdItem* parsed = (const ServoCmdItem*)parser.payload();
      assert(parsed->servo_id == servo_ids[i] && "Servo ID mismatch");
    }

    std::cout << "  ✓ Successfully verified servo ID mapping: 0-3\n\n";
  }

  std::cout << "All encoding tests passed! ✓\n";
  return 0;
}
