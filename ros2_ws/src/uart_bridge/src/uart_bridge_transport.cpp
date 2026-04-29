#include "uart_bridge_node_internal.hpp"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "shared/servo_names.hpp"

UartBridgeNode::UartBridgeNode()
    : Node("uart_bridge_node"),
      uart_fd_(-1),
      parser_(nullptr),
      handshake_complete_(false),
      stm32_system_state_(UART_SYSTEM_STATE_BOOT_CENTERING) {
  // 参数默认面向树莓派 UART1。WSL/USB 转串口调试时可在 YAML 中改成 /dev/ttyUSB*。
  declare_parameter<std::string>("uart_device", "/dev/ttyAMA0");
  declare_parameter<int>("uart_baudrate", 921600);
  declare_parameter<double>("stats_report_interval_sec", 10.0);

  std::string device = this->get_parameter("uart_device").as_string();
  int baudrate = this->get_parameter("uart_baudrate").as_int();
  double stats_interval = this->get_parameter("stats_report_interval_sec").as_double();

  uart_fd_ = open(device.c_str(), O_RDWR | O_NOCTTY);
  if (uart_fd_ < 0) {
    RCLCPP_ERROR(this->get_logger(), "Failed to open %s: %s", device.c_str(), strerror(errno));
    return;
  }

  struct termios tty;
  memset(&tty, 0, sizeof(tty));
  if (tcgetattr(uart_fd_, &tty) != 0) {
    RCLCPP_ERROR(this->get_logger(), "tcgetattr failed: %s", strerror(errno));
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

  // 原始 8N1 串口配置：8 位数据、无校验、1 位停止位、允许本地读写。
  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cflag |= (CREAD | CLOCAL);

  // 非阻塞式短超时读取，读线程可以及时响应 rclcpp::ok() 退出。
  tty.c_cc[VTIME] = 1;
  tty.c_cc[VMIN] = 0;

  if (tcsetattr(uart_fd_, TCSANOW, &tty) != 0) {
    RCLCPP_ERROR(this->get_logger(), "tcsetattr failed: %s", strerror(errno));
    close(uart_fd_);
    uart_fd_ = -1;
    return;
  }
  tcflush(uart_fd_, TCIOFLUSH);

  RCLCPP_INFO(this->get_logger(), "UART device %s opened at %d bps (protocol v%d with monitoring)",
              device.c_str(), baudrate, UART_PROTOCOL_VERSION);

  parser_ = std::make_unique<FrameParser>();
  // 解析器只负责字节流组帧；业务语义在 OnFrameReceived 中处理。
  parser_->SetFrameCallback([this](uint8_t cmd_id, const uint8_t* payload, size_t len) {
    OnFrameReceived(cmd_id, payload, len);
  });
  parser_->SetErrorCallback([this](const char* msg) {
    RCLCPP_WARN(this->get_logger(), "Parse error: %s", msg);
    frame_error_count_++;
  });

  encoder_ = std::make_unique<FrameEncoder>();

  // 使用 SensorDataQoS 接收控制命令，优先保持低延迟，过期帧可被新帧覆盖。
  servo_cmd_sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
      "/servo_cmd",
      rclcpp::SensorDataQoS(),
      [this](const sensor_msgs::msg::JointState::SharedPtr msg) {
        OnServoCmdReceived(msg);
      });

  rclcpp::QoS qos(10);
  qos.reliable();
  // 舵机状态是控制闭环的反馈，使用 reliable 避免本机进程间传输丢消息。
  servo_state_pub_ = this->create_publisher<sensor_msgs::msg::JointState>(
      "/servo_state", qos);

  stats_timer_ = this->create_wall_timer(
      std::chrono::duration<double>(stats_interval),
      [this]() { ReportStatistics(); });

  handshake_timer_ = this->create_wall_timer(
      std::chrono::milliseconds(500),
      [this]() {
        if (!handshake_complete_.load()) {
          SendInitHandshake();
        }
      });

  // 串口读取是阻塞 I/O，放到独立线程，避免占用 ROS executor。
  read_thread_ = std::thread(&UartBridgeNode::ReadLoop, this);
  SendInitHandshake();
}

UartBridgeNode::~UartBridgeNode() {
  if (read_thread_.joinable()) {
    read_thread_.join();
  }
  if (uart_fd_ >= 0) {
    close(uart_fd_);
  }
}

void UartBridgeNode::ReadLoop() {
  unsigned char buf[256];
  if (uart_fd_ < 0) {
    RCLCPP_ERROR(this->get_logger(), "UART not initialized, cannot start read loop");
    return;
  }

  while (rclcpp::ok()) {
    int n = read(uart_fd_, buf, sizeof(buf));
    if (n > 0) {
      // UART 是无边界字节流，必须逐字节推进协议状态机。
      for (int i = 0; i < n; ++i) {
        parser_->ProcessByte(buf[i]);
      }
    } else if (n < 0) {
      RCLCPP_ERROR(this->get_logger(), "UART read error: %s", strerror(errno));
      break;
    }
  }
}

uint8_t UartBridgeNode::NameToServoId(const std::string& name) {
  // 关节名映射集中放在 shared/servo_names.hpp，避免 ROS 和 STM32 顺序漂移。
  int servo_id = project_shared::servo_name_to_id(name);
  if (servo_id < 0) {
    return 0xFF;
  }
  return static_cast<uint8_t>(servo_id);
}

void UartBridgeNode::WriteFrame(const std::vector<uint8_t>& frame) {
  if (uart_fd_ < 0) {
    RCLCPP_ERROR(this->get_logger(), "UART not open, cannot send frame of %zu bytes", frame.size());
    return;
  }

  if (frame.empty()) {
    RCLCPP_WARN(this->get_logger(), "Attempt to write empty frame");
    return;
  }

  ssize_t n = write(uart_fd_, frame.data(), frame.size());
  if (n < 0) {
    RCLCPP_ERROR(this->get_logger(), "Failed to write %zu bytes to UART: %s",
                 frame.size(), strerror(errno));
  } else if (static_cast<size_t>(n) != frame.size()) {
    RCLCPP_WARN(this->get_logger(), "Partial write: %zd/%zu bytes", n, frame.size());
  } else {
    RCLCPP_DEBUG(this->get_logger(), "Successfully wrote %zu bytes to UART", frame.size());
  }
}
