#include <rclcpp/rclcpp.hpp>
#include <thread>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include "uart_bridge/frame_parser.hpp"
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

      size_t num_items = len / sizeof(ServoStateItem);
      for (size_t i = 0; i < num_items; ++i) {
        const ServoStateItem* item = reinterpret_cast<const ServoStateItem*>(payload + i * sizeof(ServoStateItem));
        float angle = item->current_angle_x10 / 10.0f;
        RCLCPP_INFO(this->get_logger(), "Got servo_state: id=%u angle=%.1f status=%u",
                    item->servo_id, angle, item->status);
      }
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
