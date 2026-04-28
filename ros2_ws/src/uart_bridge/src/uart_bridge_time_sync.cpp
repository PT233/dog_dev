#include "uart_bridge_node_internal.hpp"

#include <algorithm>

void TimestampMapper::RecordMapping(uint32_t stm32_time_ms, rclcpp::Time rpi_now) {
  std::lock_guard<std::mutex> lock(mutex_);
  mappings_.push_back({stm32_time_ms, rpi_now});
  if (mappings_.size() > MAX_MAPPINGS) {
    mappings_.pop_front();
  }
}

rclcpp::Time TimestampMapper::MapTimestamp(
    uint32_t stm32_time_ms, const rclcpp::Time& fallback_time) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (mappings_.empty()) {
    return fallback_time;
  }

  if (mappings_.size() == 1) {
    int64_t delta_ms = static_cast<int64_t>(stm32_time_ms) -
                       static_cast<int64_t>(mappings_[0].stm32_time_ms);
    return mappings_[0].rpi_time +
           rclcpp::Duration::from_nanoseconds(delta_ms * 1000000LL);
  }

  for (size_t i = 1; i < mappings_.size(); i++) {
    if (stm32_time_ms >= mappings_[i - 1].stm32_time_ms &&
        stm32_time_ms <= mappings_[i].stm32_time_ms) {
      uint32_t dt = mappings_[i].stm32_time_ms - mappings_[i - 1].stm32_time_ms;
      if (dt == 0) {
        return mappings_[i].rpi_time;
      }

      double ratio = static_cast<double>(stm32_time_ms - mappings_[i - 1].stm32_time_ms) / dt;
      rclcpp::Duration elapsed = mappings_[i].rpi_time - mappings_[i - 1].rpi_time;
      return mappings_[i - 1].rpi_time +
             rclcpp::Duration(0, static_cast<int64_t>(elapsed.nanoseconds() * ratio));
    }
  }

  return mappings_.back().rpi_time;
}

void LatencyMonitor::RecordReceiveLatency(rclcpp::Time stm32_send, rclcpp::Time rpi_receive) {
  std::lock_guard<std::mutex> lock(mutex_);
  rclcpp::Duration latency = rpi_receive - stm32_send;
  double latency_ms = latency.nanoseconds() / 1e6;

  latency_uart_min_ms_ = std::min(latency_uart_min_ms_, latency_ms);
  latency_uart_max_ms_ = std::max(latency_uart_max_ms_, latency_ms);
  latency_uart_sum_ms_ += latency_ms;
  latency_uart_count_++;
}

void LatencyMonitor::GetStats(
    double& min_ms, double& max_ms, double& avg_ms, uint32_t& count) {
  std::lock_guard<std::mutex> lock(mutex_);
  min_ms = latency_uart_min_ms_;
  max_ms = latency_uart_max_ms_;
  avg_ms = (latency_uart_count_ > 0) ? (latency_uart_sum_ms_ / latency_uart_count_) : 0.0;
  count = latency_uart_count_;
}

void LatencyMonitor::ResetStats() {
  std::lock_guard<std::mutex> lock(mutex_);
  latency_uart_min_ms_ = 1000.0;
  latency_uart_max_ms_ = 0.0;
  latency_uart_sum_ms_ = 0.0;
  latency_uart_count_ = 0;
}

FrameSequenceChecker::FrameSequenceChecker() {
  for (int i = 0; i < 4; i++) {
    seq_data_.last_seq[i] = 0xFFFF;
  }
  seq_data_.drop_count = 0;
}

bool FrameSequenceChecker::CheckSequence(uint8_t servo_id, uint16_t current_seq, uint32_t& dropped) {
  if (servo_id >= 4) {
    return false;
  }

  std::lock_guard<std::mutex> lock(seq_data_.mutex);

  if (seq_data_.last_seq[servo_id] == 0xFFFF) {
    seq_data_.last_seq[servo_id] = current_seq;
    dropped = 0;
    return true;
  }

  uint16_t expected = seq_data_.last_seq[servo_id] + 1;
  if (current_seq != expected) {
    uint32_t gap = (current_seq - expected + 65536) % 65536;
    seq_data_.drop_count += gap;
    dropped = gap;
  } else {
    dropped = 0;
  }

  seq_data_.last_seq[servo_id] = current_seq;
  return dropped == 0;
}

uint32_t FrameSequenceChecker::GetTotalDropped() {
  std::lock_guard<std::mutex> lock(seq_data_.mutex);
  return seq_data_.drop_count;
}

void FrameSequenceChecker::ResetStats() {
  std::lock_guard<std::mutex> lock(seq_data_.mutex);
  for (int i = 0; i < 4; i++) {
    seq_data_.last_seq[i] = 0xFFFF;
  }
  seq_data_.drop_count = 0;
}
