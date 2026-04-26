#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <thread>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <cmath>
#include <deque>
#include <mutex>
#include <chrono>
#include "uart_bridge/frame_parser.hpp"
#include "uart_bridge/frame_encoder.hpp"
#include "uart_bridge/uart_protocol.h"

class TimestampMapper {
private:
  struct Mapping {
    uint32_t stm32_time_ms;
    rclcpp::Time rpi_time;
  };
  std::deque<Mapping> mappings_;
  std::mutex mutex_;
  static const size_t MAX_MAPPINGS = 20;

public:
  void RecordMapping(uint32_t stm32_time_ms, rclcpp::Time rpi_now) {
    std::lock_guard<std::mutex> lock(mutex_);
    mappings_.push_back({stm32_time_ms, rpi_now});
    if (mappings_.size() > MAX_MAPPINGS) {
      mappings_.pop_front();
    }
  }

  rclcpp::Time MapTimestamp(uint32_t stm32_time_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (mappings_.empty()) {
      return rclcpp::Clock(RCL_SYSTEM_TIME).now();
    }

    if (mappings_.size() == 1) {
      uint64_t time_diff_ns = (uint64_t)stm32_time_ms * 1000000UL;
      uint64_t stm32_ref_ns = mappings_[0].stm32_time_ms * 1000000UL;
      return mappings_[0].rpi_time + rclcpp::Duration(0, time_diff_ns - stm32_ref_ns);
    }

    for (size_t i = 1; i < mappings_.size(); i++) {
      if (stm32_time_ms >= mappings_[i-1].stm32_time_ms &&
          stm32_time_ms <= mappings_[i].stm32_time_ms) {
        uint32_t dt = mappings_[i].stm32_time_ms - mappings_[i-1].stm32_time_ms;
        if (dt == 0) return mappings_[i].rpi_time;

        double ratio = (double)(stm32_time_ms - mappings_[i-1].stm32_time_ms) / dt;
        rclcpp::Duration elapsed = mappings_[i].rpi_time - mappings_[i-1].rpi_time;
        return mappings_[i-1].rpi_time +
               rclcpp::Duration(0, (int64_t)(elapsed.nanoseconds() * ratio));
      }
    }

    return mappings_.back().rpi_time;
  }
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
  static const size_t MAX_SAMPLES = 100;

  double latency_uart_min_ms_ = 1000.0;
  double latency_uart_max_ms_ = 0.0;
  double latency_uart_sum_ms_ = 0.0;
  uint32_t latency_uart_count_ = 0;

public:
  void RecordReceiveLatency(rclcpp::Time stm32_send, rclcpp::Time rpi_receive) {
    std::lock_guard<std::mutex> lock(mutex_);
    rclcpp::Duration latency = rpi_receive - stm32_send;
    double latency_ms = latency.nanoseconds() / 1e6;

    latency_uart_min_ms_ = std::min(latency_uart_min_ms_, latency_ms);
    latency_uart_max_ms_ = std::max(latency_uart_max_ms_, latency_ms);
    latency_uart_sum_ms_ += latency_ms;
    latency_uart_count_++;
  }

  void GetStats(double& min_ms, double& max_ms, double& avg_ms, uint32_t& count) {
    std::lock_guard<std::mutex> lock(mutex_);
    min_ms = latency_uart_min_ms_;
    max_ms = latency_uart_max_ms_;
    avg_ms = (latency_uart_count_ > 0) ? (latency_uart_sum_ms_ / latency_uart_count_) : 0.0;
    count = latency_uart_count_;
  }

  void ResetStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    latency_uart_min_ms_ = 1000.0;
    latency_uart_max_ms_ = 0.0;
    latency_uart_sum_ms_ = 0.0;
    latency_uart_count_ = 0;
  }
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
  FrameSequenceChecker() {
    for (int i = 0; i < 4; i++) {
      seq_data_.last_seq[i] = 0xFFFF;
    }
    seq_data_.drop_count = 0;
  }

  bool CheckSequence(uint8_t servo_id, uint16_t current_seq, uint32_t& dropped) {
    if (servo_id >= 4) return false;

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

  uint32_t GetTotalDropped() {
    std::lock_guard<std::mutex> lock(seq_data_.mutex);
    return seq_data_.drop_count;
  }

  void ResetStats() {
    std::lock_guard<std::mutex> lock(seq_data_.mutex);
    for (int i = 0; i < 4; i++) {
      seq_data_.last_seq[i] = 0xFFFF;
    }
    seq_data_.drop_count = 0;
  }
};

class UartBridgeNode : public rclcpp::Node {
public:
  UartBridgeNode() : Node("uart_bridge_node"), uart_fd_(-1), parser_(nullptr) {
    declare_parameter<std::string>("uart_device", "/dev/ttyAMA0");
    declare_parameter<int>("uart_baudrate", 921600);
    declare_parameter<double>("stats_report_interval_sec", 10.0);

    std::string device = this->get_parameter("uart_device").as_string();
    int baudrate = this->get_parameter("uart_baudrate").as_int();
    double stats_interval = this->get_parameter("stats_report_interval_sec").as_double();

    uart_fd_ = open(device.c_str(), O_RDWR | O_NOCTTY);
    if (uart_fd_ < 0) {
      RCLCPP_ERROR(this->get_logger(), "Failed to open %s", device.c_str());
      return;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(uart_fd_, &tty) != 0) {
      RCLCPP_ERROR(this->get_logger(), "tcgetattr failed");
      close(uart_fd_);
      uart_fd_ = -1;
      return;
    }

    speed_t baud;
    switch (baudrate) {
      case 115200:
        baud = B115200;
        break;
      case 921600:
        baud = B921600;
        break;
      default:
        RCLCPP_ERROR(this->get_logger(), "Unsupported baudrate: %d", baudrate);
        close(uart_fd_);
        uart_fd_ = -1;
        return;
    }

    cfsetospeed(&tty, baud);
    cfsetispeed(&tty, baud);

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= (CREAD | CLOCAL);

    tty.c_cc[VTIME] = 1;
    tty.c_cc[VMIN] = 0;

    if (tcsetattr(uart_fd_, TCSANOW, &tty) != 0) {
      RCLCPP_ERROR(this->get_logger(), "tcsetattr failed");
      close(uart_fd_);
      uart_fd_ = -1;
      return;
    }

    RCLCPP_INFO(this->get_logger(), "UART device %s opened at %d bps (v2 with monitoring)",
                device.c_str(), baudrate);

    parser_ = std::make_unique<FrameParser>();
    parser_->SetFrameCallback([this](uint8_t cmd_id, const uint8_t* payload, size_t len) {
      OnFrameReceived(cmd_id, payload, len);
    });
    parser_->SetErrorCallback([this](const char* msg) {
      RCLCPP_WARN(this->get_logger(), "Parse error: %s", msg);
      frame_error_count_++;
    });

    encoder_ = std::make_unique<FrameEncoder>();

    servo_cmd_sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
        "/servo_cmd",
        rclcpp::SensorDataQoS(),
        [this](const sensor_msgs::msg::JointState::SharedPtr msg) {
          OnServoCmdReceived(msg);
        });

    rclcpp::QoS qos(10);
    qos.reliable();
    servo_state_pub_ = this->create_publisher<sensor_msgs::msg::JointState>(
        "/servo_state", qos);

    stats_timer_ = this->create_wall_timer(
        std::chrono::duration<double>(stats_interval),
        [this]() { ReportStatistics(); });

    read_thread_ = std::thread(&UartBridgeNode::ReadLoop, this);
  }

  ~UartBridgeNode() {
    if (read_thread_.joinable()) {
      read_thread_.join();
    }
    if (uart_fd_ >= 0) {
      close(uart_fd_);
    }
  }

private:
  int uart_fd_;
  std::thread read_thread_;
  rclcpp::TimerBase::SharedPtr stats_timer_;
  std::unique_ptr<FrameParser> parser_;
  std::unique_ptr<FrameEncoder> encoder_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr servo_cmd_sub_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr servo_state_pub_;

  TimestampMapper timestamp_mapper_;
  LatencyMonitor latency_monitor_;
  FrameSequenceChecker sequence_checker_;

  uint32_t frame_error_count_ = 0;
  uint32_t frame_received_count_ = 0;

  void ReadLoop() {
    unsigned char buf[256];
    while (rclcpp::ok()) {
      int n = read(uart_fd_, buf, sizeof(buf));
      if (n > 0) {
        for (int i = 0; i < n; ++i) {
          parser_->ProcessByte(buf[i]);
        }
      }
    }
  }

  void OnFrameReceived(uint8_t cmd_id, const uint8_t* payload, size_t len) {
    rclcpp::Time t_receive = this->now();

    if (cmd_id == UART_CMD_SERVO_STATE_V2) {
      if (len % sizeof(ServoStateItem_v2) != 0) {
        RCLCPP_WARN(this->get_logger(), "Invalid servo state v2 payload size: %zu", len);
        return;
      }

      auto state_msg = std::make_shared<sensor_msgs::msg::JointState>();
      state_msg->header.frame_id = "";

      size_t num_items = len / sizeof(ServoStateItem_v2);
      const ServoStateItem_v2* first_item =
          reinterpret_cast<const ServoStateItem_v2*>(payload);

      rclcpp::Time t_stm32_send = timestamp_mapper_.MapTimestamp(first_item->timestamp_ms);
      state_msg->header.stamp = t_stm32_send;

      latency_monitor_.RecordReceiveLatency(t_stm32_send, t_receive);

      for (size_t i = 0; i < num_items; ++i) {
        const ServoStateItem_v2* item = reinterpret_cast<const ServoStateItem_v2*>(
            payload + i * sizeof(ServoStateItem_v2));

        uint32_t dropped = 0;
        bool seq_ok = sequence_checker_.CheckSequence(item->servo_id, item->frame_seq, dropped);
        if (!seq_ok && dropped > 0) {
          RCLCPP_WARN(this->get_logger(), "Frame loss detected on servo %u: %u frames lost",
                     item->servo_id, dropped);
        }

        float angle = item->current_angle_x10 / 10.0f;

        std::string name;
        if (item->servo_id == 0) name = "yaw";
        else if (item->servo_id == 1) name = "pitch";
        else if (item->servo_id == 2) name = "s2";
        else if (item->servo_id == 3) name = "s3";
        else continue;

        state_msg->name.push_back(name);
        state_msg->position.push_back(angle * M_PI / 180.0);
      }

      if (!state_msg->position.empty()) {
        servo_state_pub_->publish(*state_msg);
        frame_received_count_++;
      }

      timestamp_mapper_.RecordMapping(first_item->timestamp_ms, t_receive);
    }
    else if (cmd_id == UART_CMD_SERVO_STATE) {
      if (len % sizeof(ServoStateItem) != 0) {
        RCLCPP_WARN(this->get_logger(), "Invalid servo state v1 payload size: %zu", len);
        return;
      }

      auto state_msg = std::make_shared<sensor_msgs::msg::JointState>();
      state_msg->header.stamp = this->now();
      state_msg->header.frame_id = "";

      size_t num_items = len / sizeof(ServoStateItem);
      for (size_t i = 0; i < num_items; ++i) {
        const ServoStateItem* item = reinterpret_cast<const ServoStateItem*>(
            payload + i * sizeof(ServoStateItem));
        float angle = item->current_angle_x10 / 10.0f;

        std::string name;
        if (item->servo_id == 0) name = "yaw";
        else if (item->servo_id == 1) name = "pitch";
        else if (item->servo_id == 2) name = "s2";
        else if (item->servo_id == 3) name = "s3";
        else continue;

        state_msg->name.push_back(name);
        state_msg->position.push_back(angle * M_PI / 180.0);
      }

      if (!state_msg->position.empty()) {
        servo_state_pub_->publish(*state_msg);
        frame_received_count_++;
      }
    }
  }

  void OnServoCmdReceived(const sensor_msgs::msg::JointState::SharedPtr msg) {
    if (msg->name.empty() || msg->position.empty()) {
      return;
    }

    std::vector<ServoCmdItem> items;
    for (size_t i = 0; i < msg->name.size() && i < msg->position.size(); ++i) {
      uint8_t servo_id = NameToServoId(msg->name[i]);
      if (servo_id >= 4) {
        RCLCPP_WARN(this->get_logger(), "Unknown servo name: %s", msg->name[i].c_str());
        continue;
      }

      float angle_deg = msg->position[i] * 180.0f / M_PI;
      int16_t angle_x10 = static_cast<int16_t>(angle_deg * 10.0f);

      ServoCmdItem item;
      item.servo_id = servo_id;
      item.angle_x10 = angle_x10;
      item.duration_ms = 100;

      items.push_back(item);
    }

    if (!items.empty()) {
      auto frame = encoder_->EncodeServoControl(items.data(), items.size());
      WriteFrame(frame);
    }
  }

  uint8_t NameToServoId(const std::string& name) {
    if (name == "yaw") return 0;
    if (name == "pitch") return 1;
    if (name == "s2") return 2;
    if (name == "s3") return 3;
    return 0xFF;
  }

  void WriteFrame(const std::vector<uint8_t>& frame) {
    if (uart_fd_ < 0) {
      RCLCPP_WARN(this->get_logger(), "UART not open, cannot send frame");
      return;
    }

    ssize_t n = write(uart_fd_, frame.data(), frame.size());
    if (n < 0) {
      RCLCPP_ERROR(this->get_logger(), "Failed to write to UART");
    } else if (static_cast<size_t>(n) != frame.size()) {
      RCLCPP_WARN(this->get_logger(), "Partial write: %zd/%zu bytes", n, frame.size());
    }
  }

  void ReportStatistics() {
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
};

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UartBridgeNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
