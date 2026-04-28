#pragma once

// 视觉引导腿部控制节点
// 固定摄像头不参与转动；视觉链输出的 /pixel_error 用来生成机器人腿部动作。
// 该节点以 30Hz 定时器驱动一个简化步态控制器：
//   /pixel_error (Vector3) → 前进/转向控制量 → 4 路腿舵机 /servo_cmd (JointState, 弧度)
// 同时订阅 /servo_state 获取当前腿舵机角度反馈。

#include "geometry_msgs/msg/vector3.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "visual_servo/pid_controller.hpp"

#include <array>
#include <memory>

namespace visual_servo {

class LegMotionControllerNode : public rclcpp::Node {
 public:
  LegMotionControllerNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

 private:
  rclcpp::Subscription<geometry_msgs::msg::Vector3>::SharedPtr pixel_error_sub_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr servo_state_sub_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr servo_cmd_pub_;
  rclcpp::TimerBase::SharedPtr control_timer_;

  float pixel_error_x_ = 0.0f;
  float pixel_error_y_ = 0.0f;
  std::array<float, 4> current_leg_angles_deg_ = {90.0f, 90.0f, 90.0f, 90.0f};
  bool have_pixel_error_ = false;
  rclcpp::Time last_pixel_error_time_;

  float deadband_px_ = 5.0f;
  float min_angle_ = 10.0f;
  float max_angle_ = 170.0f;
  float neutral_angle_deg_ = 90.0f;
  float stride_amplitude_max_deg_ = 20.0f;
  float turn_bias_max_deg_ = 12.0f;
  float gait_frequency_hz_ = 2.0f;
  float target_timeout_sec_ = 0.5f;
  float control_rate_hz_ = 30.0f;
  float gait_phase_rad_ = 0.0f;

  std::unique_ptr<PIDController> pid_forward_;
  std::unique_ptr<PIDController> pid_turn_;
  rclcpp::Time last_control_time_;

  void OnPixelError(const geometry_msgs::msg::Vector3::SharedPtr msg);
  void OnServoState(const sensor_msgs::msg::JointState::SharedPtr msg);
  void OnControlTimer();
};

}  // namespace visual_servo
