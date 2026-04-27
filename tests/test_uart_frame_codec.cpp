#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

#include "uart_bridge/frame_encoder.hpp"
#include "uart_bridge/frame_parser.hpp"

namespace {

std::vector<uint8_t> BuildStm32StatusFrame() {
  ServoStateItem_v2 items[2] = {};
  items[0].servo_id = 0;
  items[0].current_angle_x10 = 900;
  items[0].status = 1;
  items[0].timestamp_ms = 1234;
  items[0].frame_seq = 7;

  items[1].servo_id = 1;
  items[1].current_angle_x10 = 450;
  items[1].status = 0;
  items[1].timestamp_ms = 1234;
  items[1].frame_seq = 7;

  const auto payload_len = static_cast<uint8_t>(sizeof(items));
  std::vector<uint8_t> frame;
  frame.push_back(UART_FRAME_HEADER_0);
  frame.push_back(UART_FRAME_HEADER_1);
  frame.push_back(UART_CMD_SERVO_STATE_V2);
  frame.push_back(payload_len);

  const auto* payload = reinterpret_cast<const uint8_t*>(items);
  frame.insert(frame.end(), payload, payload + payload_len);

  const uint16_t crc = crc16_ccitt(&frame[2], payload_len + 2);
  frame.push_back(static_cast<uint8_t>(crc & 0xFF));
  frame.push_back(static_cast<uint8_t>(crc >> 8));
  frame.push_back(UART_FRAME_TAIL);
  return frame;
}

std::vector<uint8_t> BuildStm32SystemStateFrame() {
  UartSystemStatePayload payload = {};
  payload.protocol_version = UART_PROTOCOL_VERSION;
  payload.system_state = UART_SYSTEM_STATE_ACTIVE;
  payload.uptime_ms = 1500;

  std::vector<uint8_t> frame;
  frame.push_back(UART_FRAME_HEADER_0);
  frame.push_back(UART_FRAME_HEADER_1);
  frame.push_back(UART_CMD_SYSTEM_STATE);
  frame.push_back(sizeof(payload));

  const auto* bytes = reinterpret_cast<const uint8_t*>(&payload);
  frame.insert(frame.end(), bytes, bytes + sizeof(payload));

  const uint16_t crc = crc16_ccitt(&frame[2], sizeof(payload) + 2);
  frame.push_back(static_cast<uint8_t>(crc & 0xFF));
  frame.push_back(static_cast<uint8_t>(crc >> 8));
  frame.push_back(UART_FRAME_TAIL);
  return frame;
}

}  // namespace

int main() {
  FrameEncoder encoder;
  const auto handshake = encoder.EncodeInitHandshake();
  assert(handshake.size() == 11);
  assert(handshake[0] == UART_FRAME_HEADER_0);
  assert(handshake[1] == UART_FRAME_HEADER_1);
  assert(handshake[2] == UART_CMD_INIT_HANDSHAKE);
  assert(handshake[3] == sizeof(UartHandshakePayload));
  assert(handshake.back() == UART_FRAME_TAIL);

  bool handshake_seen = false;
  FrameParser handshake_parser;
  handshake_parser.SetFrameCallback([&](uint8_t cmd_id, const uint8_t* payload, size_t len) {
    handshake_seen = true;
    assert(cmd_id == UART_CMD_INIT_HANDSHAKE);
    assert(len == sizeof(UartHandshakePayload));
    UartHandshakePayload item = {};
    std::memcpy(&item, payload, sizeof(item));
    assert(item.protocol_version == UART_PROTOCOL_VERSION);
    assert(item.requested_state == UART_SYSTEM_STATE_ACTIVE);
  });
  for (uint8_t byte : handshake) {
    handshake_parser.ProcessByte(byte);
  }
  assert(handshake_seen);

  const auto command = encoder.EncodeSingleServo(0, 900, 100);
  assert(command.size() == 12);
  assert(command[0] == UART_FRAME_HEADER_0);
  assert(command[1] == UART_FRAME_HEADER_1);
  assert(command[2] == UART_CMD_SERVO_CONTROL);
  assert(command[3] == sizeof(ServoCmdItem));
  assert(command.back() == UART_FRAME_TAIL);

  const uint16_t command_crc = crc16_ccitt(&command[2], sizeof(ServoCmdItem) + 2);
  assert(command[9] == static_cast<uint8_t>(command_crc & 0xFF));
  assert(command[10] == static_cast<uint8_t>(command_crc >> 8));

  bool command_seen = false;
  FrameParser command_parser;
  command_parser.SetFrameCallback([&](uint8_t cmd_id, const uint8_t* payload, size_t len) {
    command_seen = true;
    assert(cmd_id == UART_CMD_SERVO_CONTROL);
    assert(len == sizeof(ServoCmdItem));
    ServoCmdItem item = {};
    std::memcpy(&item, payload, sizeof(item));
    assert(item.servo_id == 0);
    assert(item.angle_x10 == 900);
    assert(item.duration_ms == 100);
  });
  for (uint8_t byte : command) {
    command_parser.ProcessByte(byte);
  }
  assert(command_seen);

  bool status_seen = false;
  const auto status = BuildStm32StatusFrame();
  FrameParser status_parser;
  status_parser.SetFrameCallback([&](uint8_t cmd_id, const uint8_t* payload, size_t len) {
    status_seen = true;
    assert(cmd_id == UART_CMD_SERVO_STATE_V2);
    assert(len == 2 * sizeof(ServoStateItem_v2));
    ServoStateItem_v2 first = {};
    std::memcpy(&first, payload, sizeof(first));
    assert(first.servo_id == 0);
    assert(first.current_angle_x10 == 900);
    assert(first.timestamp_ms == 1234);
    assert(first.frame_seq == 7);
  });
  for (uint8_t byte : status) {
    status_parser.ProcessByte(byte);
  }
  assert(status_seen);

  bool system_state_seen = false;
  const auto system_state = BuildStm32SystemStateFrame();
  FrameParser system_state_parser;
  system_state_parser.SetFrameCallback([&](uint8_t cmd_id, const uint8_t* payload, size_t len) {
    system_state_seen = true;
    assert(cmd_id == UART_CMD_SYSTEM_STATE);
    assert(len == sizeof(UartSystemStatePayload));
    UartSystemStatePayload state = {};
    std::memcpy(&state, payload, sizeof(state));
    assert(state.protocol_version == UART_PROTOCOL_VERSION);
    assert(state.system_state == UART_SYSTEM_STATE_ACTIVE);
    assert(state.uptime_ms == 1500);
  });
  for (uint8_t byte : system_state) {
    system_state_parser.ProcessByte(byte);
  }
  assert(system_state_seen);

  bool corrupt_seen = false;
  auto corrupt = status;
  corrupt[corrupt.size() - 3] ^= 0x01;
  FrameParser corrupt_parser;
  corrupt_parser.SetFrameCallback([&](uint8_t, const uint8_t*, size_t) {
    corrupt_seen = true;
  });
  for (uint8_t byte : corrupt) {
    corrupt_parser.ProcessByte(byte);
  }
  assert(!corrupt_seen);

  return 0;
}
