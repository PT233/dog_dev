#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <thread>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <cmath>
#include "uart_bridge/frame_parser.hpp"
#include "uart_bridge/frame_encoder.hpp"
#include "uart_bridge/uart_protocol.h"

class UartBridgeNode : public rclcpp::Node {
public:
  UartBridgeNode() : Node("uart_bridge_node"), uart_fd_(-1), parser_(nullptr) {
    // Declare parameters
    this->declare_parameter<std::string>("uart_device", "/dev/ttyAMA0");
    this->declare_parameter<int>("uart_baudrate", 921600);

    // Get parameters
    std::string device = this->get_parameter("uart_device").as_string();
    int baudrate = this->get_parameter("uart_baudrate").as_int();

    // Open UART device
    uart_fd_ = open(device.c_str(), O_RDWR | O_NOCTTY);
    if (uart_fd_ < 0) {
      RCLCPP_ERROR(this->get_logger(), "Failed to open %s", device.c_str());
      return;
    }

    // Configure serial port
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(uart_fd_, &tty) != 0) {
      RCLCPP_ERROR(this->get_logger(), "tcgetattr failed");
      close(uart_fd_);
      uart_fd_ = -1;
      return;
    }

    // Set baudrate
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

    // 8N1: 8 data bits, no parity, 1 stop bit
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= (CREAD | CLOCAL);

    // Non-blocking read with 100ms timeout
    tty.c_cc[VTIME] = 1;
    tty.c_cc[VMIN] = 0;

    if (tcsetattr(uart_fd_, TCSANOW, &tty) != 0) {
      RCLCPP_ERROR(this->get_logger(), "tcsetattr failed");
      close(uart_fd_);
      uart_fd_ = -1;
      return;
    }

    RCLCPP_INFO(this->get_logger(), "UART device %s opened at %d bps",
                device.c_str(), baudrate);

    // Initialize frame parser
    parser_ = std::make_unique<FrameParser>();
    parser_->SetFrameCallback([this](uint8_t cmd_id, const uint8_t* payload, size_t len) {
      OnFrameReceived(cmd_id, payload, len);
    });
    parser_->SetErrorCallback([this](const char* msg) {
      RCLCPP_WARN(this->get_logger(), "Parse error: %s", msg);
    });

    // Initialize frame encoder
    encoder_ = std::make_unique<FrameEncoder>();

    // Subscribe to servo commands
    servo_cmd_sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
        "/servo_cmd",
        rclcpp::SensorDataQoS(),
        [this](const sensor_msgs::msg::JointState::SharedPtr msg) {
          OnServoCmdReceived(msg);
        });

    // Publish servo state
    rclcpp::QoS qos = rclcpp::QoS(10);
    qos.reliable();
    servo_state_pub_ = this->create_publisher<sensor_msgs::msg::JointState>(
        "/servo_state", qos);

    // Start read thread
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
  std::unique_ptr<FrameParser> parser_;
  std::unique_ptr<FrameEncoder> encoder_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr servo_cmd_sub_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr servo_state_pub_;

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
    if (cmd_id == UART_CMD_SERVO_STATE) {
      // Parse servo state items
      if (len % sizeof(ServoStateItem) != 0) {
        RCLCPP_WARN(this->get_logger(), "Invalid servo state payload size: %zu", len);
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

        // Map servo_id to joint name
        std::string name;
        if (item->servo_id == 0) name = "yaw";
        else if (item->servo_id == 1) name = "pitch";
        else if (item->servo_id == 2) name = "s2";
        else if (item->servo_id == 3) name = "s3";
        else continue;

        state_msg->name.push_back(name);
        // Convert degrees*10 back to radians
        state_msg->position.push_back(angle * M_PI / 180.0);
      }

      if (!state_msg->position.empty()) {
        servo_state_pub_->publish(*state_msg);
        RCLCPP_DEBUG(this->get_logger(), "Published servo state with %zu joints",
                     state_msg->position.size());
      }
    }
  }

  void OnServoCmdReceived(const sensor_msgs::msg::JointState::SharedPtr msg) {
    if (msg->name.empty() || msg->position.empty()) {
      return;
    }

    // Build servo command items from JointState
    std::vector<ServoCmdItem> items;
    for (size_t i = 0; i < msg->name.size() && i < msg->position.size(); ++i) {
      uint8_t servo_id = NameToServoId(msg->name[i]);
      if (servo_id >= 4) {
        RCLCPP_WARN(this->get_logger(), "Unknown servo name: %s", msg->name[i].c_str());
        continue;
      }

      // Convert radians to degrees * 10
      float angle_deg = msg->position[i] * 180.0f / M_PI;
      int16_t angle_x10 = static_cast<int16_t>(angle_deg * 10.0f);

      ServoCmdItem item;
      item.servo_id = servo_id;
      item.angle_x10 = angle_x10;
      item.duration_ms = 100;  // Fixed duration for now

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
    return 0xFF;  // Invalid
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
    } else {
      RCLCPP_DEBUG(this->get_logger(), "Sent frame: %zu bytes", frame.size());
    }
  }
};

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UartBridgeNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
