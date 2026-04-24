#pragma once

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/vector3.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "visual_servo/pid_controller.hpp"
#include <memory>

namespace visual_servo {

class VisualServoNode : public rclcpp::Node {
public:
  VisualServoNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
  // Subscriptions and publishers
  rclcpp::Subscription<geometry_msgs::msg::Vector3>::SharedPtr pixel_error_sub_;
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr servo_state_sub_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr servo_cmd_pub_;

  // Timer for control loop
  rclcpp::TimerBase::SharedPtr control_timer_;

  // Control state
  float current_yaw_ = 90.0f;    // Initial position (degrees)
  float current_pitch_ = 90.0f;
  float pixel_error_x_ = 0.0f;
  float pixel_error_y_ = 0.0f;

  // Control parameters
  float deadband_px_ = 5.0f;
  float min_angle_ = 10.0f;
  float max_angle_ = 170.0f;
  float control_rate_hz_ = 30.0f;

  // PID controllers
  std::unique_ptr<PIDController> pid_yaw_;
  std::unique_ptr<PIDController> pid_pitch_;

  // Last control time for dt calculation
  rclcpp::Time last_control_time_;

  // Callbacks
  void OnPixelError(const geometry_msgs::msg::Vector3::SharedPtr msg);
  void OnServoState(const sensor_msgs::msg::JointState::SharedPtr msg);
  void OnControlTimer();
};

}  // namespace visual_servo
