#ifndef UART_BRIDGE_NODE_INTERNAL_HPP_
#define UART_BRIDGE_NODE_INTERNAL_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "uart_bridge/frame_encoder.hpp"
#include "uart_bridge/frame_parser.hpp"
#include "uart_bridge/uart_protocol.h"

class TimestampMapper {
 private:
  struct Mapping {
    uint32_t stm32_time_ms;
    rclcpp::Time rpi_time;
  };

  std::deque<Mapping> mappings_;
  std::mutex mutex_;
  static constexpr size_t MAX_MAPPINGS = 20;

 public:
  void RecordMapping(uint32_t stm32_time_ms, rclcpp::Time rpi_now);
  rclcpp::Time MapTimestamp(uint32_t stm32_time_ms, const rclcpp::Time& fallback_time);
};

class LatencyMonitor {
 private:
  struct LatencySample {
    rclcpp::Time t_stm32_send;
    rclcpp::Time t_rpi_receive;
    rclcpp::Time t_rpi_send_cmd;
    rclcpp::Time t_wsl2_recv_cmd;
  };

  std::deque<LatencySample> samples_;
  std::mutex mutex_;
  static constexpr size_t MAX_SAMPLES = 100;

  double latency_uart_min_ms_ = 1000.0;
  double latency_uart_max_ms_ = 0.0;
  double latency_uart_sum_ms_ = 0.0;
  uint32_t latency_uart_count_ = 0;

 public:
  void RecordReceiveLatency(rclcpp::Time stm32_send, rclcpp::Time rpi_receive);
  void GetStats(double& min_ms, double& max_ms, double& avg_ms, uint32_t& count);
  void ResetStats();
};

class FrameSequenceChecker {
 private:
  struct SequenceData {
    uint16_t last_seq[4];
    uint32_t drop_count;
    std::mutex mutex;
  };

  SequenceData seq_data_;

 public:
  FrameSequenceChecker();
  bool CheckSequence(uint8_t servo_id, uint16_t current_seq, uint32_t& dropped);
  uint32_t GetTotalDropped();
  void ResetStats();
};

class UartBridgeNode : public rclcpp::Node {
 public:
  UartBridgeNode();
  ~UartBridgeNode() override;

 private:
  int uart_fd_;
  std::thread read_thread_;
  rclcpp::TimerBase::SharedPtr stats_timer_;
  rclcpp::TimerBase::SharedPtr handshake_timer_;
  std::unique_ptr<FrameParser> parser_;
  std::unique_ptr<FrameEncoder> encoder_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr servo_cmd_sub_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr servo_state_pub_;

  TimestampMapper timestamp_mapper_;
  LatencyMonitor latency_monitor_;
  FrameSequenceChecker sequence_checker_;

  std::atomic<bool> handshake_complete_;
  std::atomic<uint8_t> stm32_system_state_;
  uint32_t handshake_tx_count_ = 0;
  uint32_t frame_error_count_ = 0;
  uint32_t frame_received_count_ = 0;

  void ReadLoop();
  const char* SystemStateToString(uint8_t state) const;
  void SendInitHandshake();
  void OnSystemStateReceived(const uint8_t* payload, size_t len);
  void OnFrameReceived(uint8_t cmd_id, const uint8_t* payload, size_t len);
  void OnServoCmdReceived(const sensor_msgs::msg::JointState::SharedPtr msg);
  uint8_t NameToServoId(const std::string& name);
  void WriteFrame(const std::vector<uint8_t>& frame);
  void ReportStatistics();
};

#endif  // UART_BRIDGE_NODE_INTERNAL_HPP_
