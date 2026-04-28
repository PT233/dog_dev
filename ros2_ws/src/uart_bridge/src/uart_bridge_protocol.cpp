#include "uart_bridge_node_internal.hpp"

#include <cmath>
#include <cstring>

#include "shared/servo_names.hpp"

const char* UartBridgeNode::SystemStateToString(uint8_t state) const {
  switch (state) {
    case UART_SYSTEM_STATE_BOOT_CENTERING:
      return "BOOT_CENTERING";
    case UART_SYSTEM_STATE_WAITING_CONNECTION:
      return "WAITING_CONNECTION";
    case UART_SYSTEM_STATE_ACTIVE:
      return "ACTIVE";
    case UART_SYSTEM_STATE_ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

void UartBridgeNode::SendInitHandshake() {
  if (handshake_complete_.load()) {
    return;
  }

  auto frame = encoder_->EncodeInitHandshake();
  WriteFrame(frame);
  handshake_tx_count_++;

  if (handshake_tx_count_ == 1) {
    RCLCPP_INFO(this->get_logger(), "Sent STM32 init handshake");
  } else {
    RCLCPP_DEBUG(this->get_logger(), "Retried STM32 init handshake (%u)",
                 handshake_tx_count_);
  }
}

void UartBridgeNode::OnSystemStateReceived(const uint8_t* payload, size_t len) {
  if (len != sizeof(UartSystemStatePayload)) {
    RCLCPP_WARN(this->get_logger(), "Invalid STM32 system state payload size: %zu", len);
    return;
  }

  UartSystemStatePayload state = {};
  std::memcpy(&state, payload, sizeof(state));

  if (state.protocol_version != UART_PROTOCOL_VERSION) {
    RCLCPP_WARN(this->get_logger(),
                "STM32 protocol version mismatch: bridge=%u stm32=%u",
                UART_PROTOCOL_VERSION, state.protocol_version);
    return;
  }

  uint8_t previous = stm32_system_state_.exchange(state.system_state);
  if (state.system_state == UART_SYSTEM_STATE_ACTIVE) {
    bool was_complete = handshake_complete_.exchange(true);
    if (!was_complete) {
      if (handshake_timer_) {
        handshake_timer_->cancel();
      }
      RCLCPP_INFO(this->get_logger(),
                  "STM32 handshake complete: state=%s uptime=%u ms",
                  SystemStateToString(state.system_state), state.uptime_ms);
    }
    return;
  }

  if (previous != state.system_state) {
    RCLCPP_INFO(this->get_logger(),
                "STM32 state=%s uptime=%u ms; waiting before enabling servo commands",
                SystemStateToString(state.system_state), state.uptime_ms);
  }
}

void UartBridgeNode::OnFrameReceived(uint8_t cmd_id, const uint8_t* payload, size_t len) {
  rclcpp::Time t_receive = this->get_clock()->now();

  if (cmd_id == UART_CMD_SYSTEM_STATE) {
    OnSystemStateReceived(payload, len);
  } else if (cmd_id == UART_CMD_SERVO_STATE_V2) {
    if (len % sizeof(ServoStateItem_v2) != 0) {
      RCLCPP_WARN(this->get_logger(), "Invalid servo state v2 payload size: %zu", len);
      return;
    }

    auto state_msg = std::make_shared<sensor_msgs::msg::JointState>();
    state_msg->header.frame_id = "";

    size_t num_items = len / sizeof(ServoStateItem_v2);
    const ServoStateItem_v2* first_item =
        reinterpret_cast<const ServoStateItem_v2*>(payload);

    rclcpp::Time t_stm32_send =
        timestamp_mapper_.MapTimestamp(first_item->timestamp_ms, t_receive);
    state_msg->header.stamp = t_stm32_send;

    latency_monitor_.RecordReceiveLatency(t_stm32_send, t_receive);

    for (size_t i = 0; i < num_items; ++i) {
      const ServoStateItem_v2* item = reinterpret_cast<const ServoStateItem_v2*>(
          payload + i * sizeof(ServoStateItem_v2));

      if (item->servo_id >= 4) {
        RCLCPP_WARN(this->get_logger(), "Invalid servo_id: %u (expected 0-3)", item->servo_id);
        continue;
      }

      uint32_t dropped = 0;
      bool seq_ok = sequence_checker_.CheckSequence(item->servo_id, item->frame_seq, dropped);
      if (!seq_ok && dropped > 0) {
        RCLCPP_WARN(this->get_logger(), "Frame loss detected on servo %u: %u frames lost",
                    item->servo_id, dropped);
      }

      float angle = item->current_angle_x10 / 10.0f;

      const char* name = project_shared::servo_id_to_name(item->servo_id);
      if (name == nullptr) {
        continue;
      }

      state_msg->name.emplace_back(name);
      state_msg->position.push_back(angle * M_PI / 180.0);
    }

    if (!state_msg->position.empty()) {
      servo_state_pub_->publish(*state_msg);
      frame_received_count_++;
    }

    timestamp_mapper_.RecordMapping(first_item->timestamp_ms, t_receive);
  } else if (cmd_id == UART_CMD_SERVO_STATE) {
    if (len % sizeof(ServoStateItem) != 0) {
      RCLCPP_WARN(this->get_logger(), "Invalid servo state v1 payload size: %zu", len);
      return;
    }

    auto state_msg = std::make_shared<sensor_msgs::msg::JointState>();
    state_msg->header.stamp = this->get_clock()->now();
    state_msg->header.frame_id = "";

    size_t num_items = len / sizeof(ServoStateItem);
    for (size_t i = 0; i < num_items; ++i) {
      const ServoStateItem* item = reinterpret_cast<const ServoStateItem*>(
          payload + i * sizeof(ServoStateItem));
      float angle = item->current_angle_x10 / 10.0f;

      const char* name = project_shared::servo_id_to_name(item->servo_id);
      if (name == nullptr) {
        continue;
      }

      state_msg->name.emplace_back(name);
      state_msg->position.push_back(angle * M_PI / 180.0);
    }

    if (!state_msg->position.empty()) {
      servo_state_pub_->publish(*state_msg);
      frame_received_count_++;
    }
  }
}

void UartBridgeNode::OnServoCmdReceived(const sensor_msgs::msg::JointState::SharedPtr msg) {
  if (!handshake_complete_.load()) {
    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000,
                         "Dropping /servo_cmd until STM32 handshake completes (state=%s)",
                         SystemStateToString(stm32_system_state_.load()));
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
    uint8_t servo_id = NameToServoId(msg->name[i]);
    if (servo_id >= 4) {
      RCLCPP_WARN(this->get_logger(), "Unknown servo name: %s", msg->name[i].c_str());
      continue;
    }

    float angle_deg = msg->position[i] * 180.0f / M_PI;
    if (angle_deg < -180.0f || angle_deg > 180.0f) {
      RCLCPP_WARN(this->get_logger(), "Servo %s angle out of range: %.1f degrees",
                  msg->name[i].c_str(), angle_deg);
    }

    int16_t angle_x10 = static_cast<int16_t>(angle_deg * 10.0f);

    ServoCmdItem item;
    item.servo_id = servo_id;
    item.angle_x10 = angle_x10;
    item.duration_ms = 100;

    items.push_back(item);
  }

  if (items.empty()) {
    RCLCPP_WARN(this->get_logger(), "No valid servo commands after filtering");
    return;
  }

  auto frame = encoder_->EncodeServoControl(items.data(), items.size());
  WriteFrame(frame);
}

void UartBridgeNode::ReportStatistics() {
  double min_ms, max_ms, avg_ms;
  uint32_t count;
  latency_monitor_.GetStats(min_ms, max_ms, avg_ms, count);

  RCLCPP_INFO(this->get_logger(),
      "=== UART Bridge Statistics ===\n"
      "  Frames received: %u\n"
      "  Frame errors: %u\n"
      "  Total packet loss: %u frames\n"
      "  Latency UART RX: min=%.2f ms, max=%.2f ms, avg=%.2f ms (samples: %u)",
      frame_received_count_,
      frame_error_count_,
      sequence_checker_.GetTotalDropped(),
      min_ms, max_ms, avg_ms, count);
}
