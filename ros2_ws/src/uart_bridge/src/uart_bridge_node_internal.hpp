#ifndef UART_BRIDGE_NODE_INTERNAL_HPP_
#define UART_BRIDGE_NODE_INTERNAL_HPP_

// uart_bridge 内部实现头
//
// 该模块处在 ROS 2 与 STM32 UART 协议之间：
// - 订阅 /servo_cmd，将 JointState 转为二进制舵机控制帧；
// - 读取 STM32 状态帧，发布 /servo_state；
// - 握手完成前阻止控制指令下发，保护 MCU 上电归中阶段；
// - 记录时间戳映射、串口延迟和帧序号，用于现场诊断链路质量。

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
    uint32_t stm32_time_ms;  // STM32 状态帧里的毫秒时间戳
    rclcpp::Time rpi_time;   // 树莓派/ROS 接收该帧时的本地时间
  };

  std::deque<Mapping> mappings_;
  std::mutex mutex_;
  static constexpr size_t MAX_MAPPINGS = 20;  // 只保留最近映射，避免长期运行内存增长

 public:
  // 记录一对 STM32 时间与 ROS 时间，用于后续把 MCU 时间戳投影到 ROS 时间轴。
  void RecordMapping(uint32_t stm32_time_ms, rclcpp::Time rpi_now);

  // 将 STM32 时间戳映射为 ROS 时间；映射不足时使用 fallback_time。
  rclcpp::Time MapTimestamp(uint32_t stm32_time_ms, const rclcpp::Time& fallback_time);
};

class LatencyMonitor {
 private:
  struct LatencySample {
    rclcpp::Time t_stm32_send;     // STM32 打包状态帧时间
    rclcpp::Time t_rpi_receive;    // 本节点收到状态帧时间
    rclcpp::Time t_rpi_send_cmd;   // 预留：本节点下发控制帧时间
    rclcpp::Time t_wsl2_recv_cmd;  // 预留：跨主机调试时的命令接收时间
  };

  std::deque<LatencySample> samples_;
  std::mutex mutex_;
  static constexpr size_t MAX_SAMPLES = 100;

  double latency_uart_min_ms_ = 1000.0;
  double latency_uart_max_ms_ = 0.0;
  double latency_uart_sum_ms_ = 0.0;
  uint32_t latency_uart_count_ = 0;

 public:
  // 记录一条 STM32 -> ROS 的接收延迟样本。
  void RecordReceiveLatency(rclcpp::Time stm32_send, rclcpp::Time rpi_receive);

  // 输出当前统计窗口的最小、最大、平均延迟和样本数。
  void GetStats(double& min_ms, double& max_ms, double& avg_ms, uint32_t& count);
  void ResetStats();
};

class FrameSequenceChecker {
 private:
  struct SequenceData {
    uint16_t last_seq[4];  // 每路舵机最近一次收到的帧序号
    uint32_t drop_count;   // 累计估算丢帧数
    std::mutex mutex;
  };

  SequenceData seq_data_;

 public:
  FrameSequenceChecker();

  // 检查某路舵机状态帧序号是否连续，dropped 返回本次发现的缺口。
  bool CheckSequence(uint8_t servo_id, uint16_t current_seq, uint32_t& dropped);
  uint32_t GetTotalDropped();
  void ResetStats();
};

class UartBridgeNode : public rclcpp::Node {
 public:
  UartBridgeNode();
  ~UartBridgeNode() override;

 private:
  int uart_fd_;              // Linux 串口文件描述符
  std::thread read_thread_;  // 阻塞读取 UART 的后台线程
  rclcpp::TimerBase::SharedPtr stats_timer_;
  rclcpp::TimerBase::SharedPtr handshake_timer_;
  std::unique_ptr<FrameParser> parser_;
  std::unique_ptr<FrameEncoder> encoder_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr servo_cmd_sub_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr servo_state_pub_;

  TimestampMapper timestamp_mapper_;
  LatencyMonitor latency_monitor_;
  FrameSequenceChecker sequence_checker_;

  std::atomic<bool> handshake_complete_;    // 只有收到 STM32 ACTIVE 后才置 true
  std::atomic<uint8_t> stm32_system_state_;  // 最近一次 STM32 系统状态
  uint32_t handshake_tx_count_ = 0;
  uint32_t frame_error_count_ = 0;
  uint32_t frame_received_count_ = 0;

  // UART 读线程入口：读到的每个字节都交给 FrameParser 状态机。
  void ReadLoop();
  const char* SystemStateToString(uint8_t state) const;

  // 周期发送握手帧，直到 STM32 回报 ACTIVE。
  void SendInitHandshake();
  void OnSystemStateReceived(const uint8_t* payload, size_t len);
  void OnFrameReceived(uint8_t cmd_id, const uint8_t* payload, size_t len);

  // /servo_cmd 回调：JointState(rad) -> ServoCmdItem(deg*10) -> UART 帧。
  void OnServoCmdReceived(const sensor_msgs::msg::JointState::SharedPtr msg);
  uint8_t NameToServoId(const std::string& name);
  void WriteFrame(const std::vector<uint8_t>& frame);
  void ReportStatistics();
};

#endif  // UART_BRIDGE_NODE_INTERNAL_HPP_
