#include "kernel/mpu6050/mpu6050_lab_uapi.h"
#include "shared/ros2_single_node_main.hpp"

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"

#include <fcntl.h>
#include <glob.h>
#include <poll.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <string>
#include <thread>

namespace imu_node
{

namespace
{
constexpr double kGravity = 9.80665;
constexpr double kDegToRad = 3.14159265358979323846 / 180.0;
constexpr double kAccelScale = kGravity / MPU6050_LAB_ACCEL_LSB_PER_G;
constexpr double kGyroScale = kDegToRad / MPU6050_LAB_GYRO_LSB_PER_DPS;

int64_t monotonic_ns()
{
  timespec ts{};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<int64_t>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
}

// 未指定设备时取第一个 /dev/mpu6050_*（驱动按 <bus>-<addr> 命名）。
std::string find_device()
{
  glob_t g{};
  std::string found;
  if (glob("/dev/mpu6050_*", 0, nullptr, &g) == 0 && g.gl_pathc > 0) {
    found = g.gl_pathv[0];
  }
  globfree(&g);
  return found;
}
}  // namespace

class ImuNode : public rclcpp::Node
{
public:
  explicit ImuNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
  : Node("imu_node", options)
  {
    device_ = declare_parameter<std::string>("device", "");
    frame_id_ = declare_parameter<std::string>("frame_id", "imu_link");
    if (device_.empty()) {
      device_ = find_device();
    }
    if (device_.empty()) {
      throw std::runtime_error("no /dev/mpu6050_* device; is mpu6050_lab loaded?");
    }

    // open() 会让驱动通过 runtime PM 唤醒芯片并开始采样。
    fd_ = open(device_.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd_ < 0) {
      throw std::runtime_error("open " + device_ + ": " + std::strerror(errno));
    }

    pub_ = create_publisher<sensor_msgs::msg::Imu>("~/output/imu_raw", rclcpp::SensorDataQoS());
    RCLCPP_INFO(get_logger(), "imu_node started on %s", device_.c_str());
    reader_ = std::thread(&ImuNode::read_loop, this);
  }

  ~ImuNode() override
  {
    running_ = false;
    if (reader_.joinable()) {
      reader_.join();
    }
    if (fd_ >= 0) {
      close(fd_);
    }
  }

private:
  // 驱动时间戳是硬中断里取的 CLOCK_MONOTONIC；换算到 ROS 时间只加一个偏移，
  // 不用本线程被调度到的时刻，避免把用户态调度抖动带进 header.stamp。
  int64_t ros_offset_ns()
  {
    return get_clock()->now().nanoseconds() - monotonic_ns();
  }

  void read_loop()
  {
    std::array<mpu6050_lab_sample, 32> buf{};
    int64_t offset = ros_offset_ns();
    int64_t last_offset_refresh = monotonic_ns();

    while (running_ && rclcpp::ok()) {
      pollfd p{fd_, POLLIN, 0};
      const int ready = poll(&p, 1, 200);
      if (ready < 0 && errno != EINTR) {
        RCLCPP_ERROR(get_logger(), "poll: %s", std::strerror(errno));
        break;
      }
      if (ready <= 0) {
        continue;
      }
      if (p.revents & (POLLHUP | POLLERR)) {
        RCLCPP_ERROR(get_logger(), "%s went away (driver unbound)", device_.c_str());
        break;
      }

      const ssize_t n = read(fd_, buf.data(), sizeof(buf));
      if (n < 0) {
        if (errno == EAGAIN || errno == EINTR) {
          continue;
        }
        RCLCPP_ERROR(get_logger(), "read: %s", std::strerror(errno));
        break;
      }

      const int64_t now = monotonic_ns();
      if (now - last_offset_refresh > 10LL * 1000000000LL) {
        offset = ros_offset_ns();
        last_offset_refresh = now;
      }

      const size_t count = static_cast<size_t>(n) / sizeof(mpu6050_lab_sample);
      for (size_t i = 0; i < count; ++i) {
        publish(buf[i], offset);
      }
    }
  }

  void publish(const mpu6050_lab_sample & s, int64_t offset)
  {
    if (s.dropped_before != 0) {
      dropped_ += s.dropped_before;
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 2000,
        "driver dropped samples, total %llu", static_cast<unsigned long long>(dropped_));
    }

    auto msg = std::make_unique<sensor_msgs::msg::Imu>();
    msg->header.stamp = rclcpp::Time(static_cast<int64_t>(s.timestamp_ns) + offset);
    msg->header.frame_id = frame_id_;
    msg->linear_acceleration.x = s.accel[0] * kAccelScale;
    msg->linear_acceleration.y = s.accel[1] * kAccelScale;
    msg->linear_acceleration.z = s.accel[2] * kAccelScale;
    msg->angular_velocity.x = s.gyro[0] * kGyroScale;
    msg->angular_velocity.y = s.gyro[1] * kGyroScale;
    msg->angular_velocity.z = s.gyro[2] * kGyroScale;
    msg->orientation_covariance[0] = -1.0;  // REP-145：不提供姿态
    pub_->publish(std::move(msg));
  }

  std::string device_;
  std::string frame_id_;
  int fd_{-1};
  std::atomic<bool> running_{true};
  uint64_t dropped_{0};
  std::thread reader_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr pub_;
};

}  // namespace imu_node

int main(int argc, char * argv[])
{
  // 启动 IMU 节点，把内核驱动的原始样本换算成 SI 单位后发布。
  return project_shared::spin_single_node_main<imu_node::ImuNode>(argc, argv);
}
