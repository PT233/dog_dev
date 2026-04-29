#include "uart_bridge_node_internal.hpp"

#include <cmath>
#include <cstring>

#include "shared/servo_names.hpp"

namespace uart_bridge
{

const char * UartBridgeNode::system_state_to_string(uint8_t state) const
{
  switch (state) {
    case kUartSystemStateBootCentering:
      return "BOOT_CENTERING";
    case kUartSystemStateWaitingConnection:
      return "WAITING_CONNECTION";
    case kUartSystemStateActive:
      return "ACTIVE";
    case kUartSystemStateError:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

void UartBridgeNode::send_init_handshake()
{
  if (handshake_complete_.load()) {
    return;
  }

  // STM32 只有在上电归中结束并收到版本匹配握手后才进入 ACTIVE。
  auto frame = encoder_->encode_init_handshake();
  write_frame(frame);
  handshake_tx_count_++;

  if (handshake_tx_count_ == 1) {
    RCLCPP_INFO(this->get_logger(), "Sent STM32 init handshake");
  } else {
    RCLCPP_DEBUG(this->get_logger(), "Retried STM32 init handshake (%u)",
                 handshake_tx_count_);
  }
}

void UartBridgeNode::system_state_callback(const uint8_t * payload, size_t len)
{
  if (len != sizeof(UartSystemStatePayload)) {
    RCLCPP_WARN(this->get_logger(), "Invalid STM32 system state payload size: %zu", len);
    return;
  }

  UartSystemStatePayload state = {};
  std::memcpy(&state, payload, sizeof(state));

  if (state.protocol_version != UART_PROTOCOL_VERSION) {
    // 版本不一致时拒绝认为握手完成，避免按错误 payload 格式解释控制帧。
    RCLCPP_WARN(this->get_logger(),
                "STM32 protocol version mismatch: bridge=%u stm32=%u",
                UART_PROTOCOL_VERSION, state.protocol_version);
    return;
  }

  uint8_t previous = stm32_system_state_.exchange(state.system_state);
  if (state.system_state == kUartSystemStateActive) {
    // 第一次看到 ACTIVE 时停止握手重试，后续舵机命令才会真正下发。
    bool was_complete = handshake_complete_.exchange(true);
    if (!was_complete) {
      if (handshake_timer_) {
        handshake_timer_->cancel();
      }
      RCLCPP_INFO(this->get_logger(),
                  "STM32 handshake complete: state=%s uptime=%u ms",
                  system_state_to_string(state.system_state), state.uptime_ms);
    }
    return;
  }

  if (previous != state.system_state) {
    // 非 ACTIVE 状态只在变化时打印，避免 20Hz 状态帧刷屏。
    RCLCPP_INFO(this->get_logger(),
                "STM32 state=%s uptime=%u ms; waiting before enabling servo commands",
                system_state_to_string(state.system_state), state.uptime_ms);
  }
}

void UartBridgeNode::frame_callback(uint8_t cmd_id, const uint8_t * payload, size_t len)
{
  rclcpp::Time t_receive = this->get_clock()->now();

  if (cmd_id == kUartCmdSystemState) {
    // 系统状态帧用于握手闭环和启动阶段可观测性。
    system_state_callback(payload, len);
  } else if (cmd_id == kUartCmdServoStateV2) {
    if (len % sizeof(ServoStateItemV2) != 0) {
      RCLCPP_WARN(this->get_logger(), "Invalid servo state v2 payload size: %zu", len);
      return;
    }

    // v2 状态帧携带 STM32 时间戳和帧序号，可用于延迟和丢帧诊断。
    auto state_msg = std::make_shared<sensor_msgs::msg::JointState>();
    state_msg->header.frame_id = "";

    size_t num_items = len / sizeof(ServoStateItemV2);
    const ServoStateItemV2 * first_item =
      reinterpret_cast<const ServoStateItemV2 *>(payload);

    // 把 STM32 毫秒时间映射到 ROS 时间，尽量让 servo_state 时间戳反映采样时刻。
    rclcpp::Time t_stm32_send =
      timestamp_mapper_.map_timestamp(first_item->timestamp_ms, t_receive);
    state_msg->header.stamp = t_stm32_send;

    latency_monitor_.record_receive_latency(t_stm32_send, t_receive);

    for (size_t i = 0; i < num_items; ++i) {
      const ServoStateItemV2 * item = reinterpret_cast<const ServoStateItemV2 *>(
        payload + i * sizeof(ServoStateItemV2));

      if (item->servo_id >= 4) {
        RCLCPP_WARN(this->get_logger(), "Invalid servo_id: %u (expected 0-3)", item->servo_id);
        continue;
      }

      uint32_t dropped = 0;
      bool seq_ok = sequence_checker_.check_sequence(item->servo_id, item->frame_seq, dropped);
      if (!seq_ok && dropped > 0) {
        RCLCPP_WARN(this->get_logger(), "Frame loss detected on servo %u: %u frames lost",
                    item->servo_id, dropped);
      }

      // 协议中角度以“度 ×10”传输；ROS JointState 要求弧度。
      float angle = item->current_angle_x10 / 10.0f;

      const char * name = project_shared::servo_id_to_name(item->servo_id);
      if (name == nullptr) {
        continue;
      }

      state_msg->name.emplace_back(name);
      state_msg->position.push_back(angle * M_PI / 180.0);
    }

    if (!state_msg->position.empty()) {
      servo_state_publisher_->publish(*state_msg);
      frame_received_count_++;
    }

    // 本帧接收完成后记录新的时间映射锚点，供下一帧插值使用。
    timestamp_mapper_.record_mapping(first_item->timestamp_ms, t_receive);
  } else if (cmd_id == kUartCmdServoState) {
    if (len % sizeof(ServoStateItem) != 0) {
      RCLCPP_WARN(this->get_logger(), "Invalid servo state v1 payload size: %zu", len);
      return;
    }

    auto state_msg = std::make_shared<sensor_msgs::msg::JointState>();
    state_msg->header.stamp = this->get_clock()->now();
    state_msg->header.frame_id = "";

    size_t num_items = len / sizeof(ServoStateItem);
    for (size_t i = 0; i < num_items; ++i) {
      const ServoStateItem * item = reinterpret_cast<const ServoStateItem *>(
        payload + i * sizeof(ServoStateItem));
      // v1 兼容帧无时间戳，只能使用本机当前时间。
      float angle = item->current_angle_x10 / 10.0f;

      const char * name = project_shared::servo_id_to_name(item->servo_id);
      if (name == nullptr) {
        continue;
      }

      state_msg->name.emplace_back(name);
      state_msg->position.push_back(angle * M_PI / 180.0);
    }

    if (!state_msg->position.empty()) {
      servo_state_publisher_->publish(*state_msg);
      frame_received_count_++;
    }
  }
}

void UartBridgeNode::servo_command_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
  if (!handshake_complete_.load()) {
    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
                         "Dropping ~/input/servo_command until STM32 handshake completes (state=%s)",
                         system_state_to_string(stm32_system_state_.load()));
    return;
  }

  if (!msg) {
    RCLCPP_ERROR(this->get_logger(), "Received null servo command message");
    return;
  }

  if (msg->name.empty() || msg->position.empty()) {
    RCLCPP_WARN(this->get_logger(), "Received empty servo command (names:%zu, positions:%zu)",
                msg->name.size(), msg->position.size());
    return;
  }

  if (msg->name.size() != msg->position.size()) {
    RCLCPP_WARN(this->get_logger(), "Servo command mismatch: %zu names vs %zu positions",
                msg->name.size(), msg->position.size());
  }

  std::vector<ServoCmdItem> items;
  for (size_t i = 0; i < msg->name.size() && i < msg->position.size(); ++i) {
    // JointState 的 name 决定舵机编号，position 单位是弧度。
    uint8_t servo_id = servo_id_by_name(msg->name[i]);
    if (servo_id >= 4) {
      RCLCPP_WARN(this->get_logger(), "Unknown servo name: %s", msg->name[i].c_str());
      continue;
    }

    float angle_deg = msg->position[i] * 180.0f / M_PI;
    if (angle_deg < -180.0f || angle_deg > 180.0f) {
      RCLCPP_WARN(this->get_logger(), "Servo %s angle out of range: %.1f degrees",
                  msg->name[i].c_str(), angle_deg);
    }

    // STM32 协议用 int16 传 “度 ×10”，减少 payload 长度并避免跨语言浮点差异。
    int16_t angle_x10 = static_cast<int16_t>(angle_deg * 10.0f);

    ServoCmdItem item;
    item.servo_id = servo_id;
    item.angle_x10 = angle_x10;
    item.duration_ms = 100;  // 当前 ROS 控制周期默认给 100ms 轨迹过渡。

    items.push_back(item);
  }

  if (items.empty()) {
    RCLCPP_WARN(this->get_logger(), "No valid servo commands after filtering");
    return;
  }

  auto frame = encoder_->encode_servo_control(items.data(), items.size());
  write_frame(frame);
}

void UartBridgeNode::report_statistics()
{
  double min_ms, max_ms, avg_ms;
  uint32_t count;
  latency_monitor_.stats(min_ms, max_ms, avg_ms, count);

  RCLCPP_INFO(this->get_logger(),
      "=== UART Bridge Statistics ===\n"
      "  Frames received: %u\n"
      "  Frame errors: %u\n"
      "  Total packet loss: %u frames\n"
      "  Latency UART RX: min=%.2f ms, max=%.2f ms, avg=%.2f ms (samples: %u)",
      frame_received_count_,
      frame_error_count_,
      sequence_checker_.total_dropped(),
      min_ms, max_ms, avg_ms, count);
}

}  // namespace uart_bridge
