#include "visual_servo/visual_servo_node.hpp"
#include <cmath>
#include <algorithm>

namespace visual_servo {

VisualServoNode::VisualServoNode(const rclcpp::NodeOptions& options)
    : rclcpp::Node("visual_servo_node", options) {
  // Declare and get parameters
  this->declare_parameter<float>("control_rate_hz", 30.0f);
  this->declare_parameter<float>("deadband_px", 5.0f);
  this->declare_parameter<float>("min_angle", 10.0f);
  this->declare_parameter<float>("max_angle", 170.0f);

  // Yaw PID parameters
  this->declare_parameter<float>("yaw.kp", 0.05f);
  this->declare_parameter<float>("yaw.ki", 0.001f);
  this->declare_parameter<float>("yaw.kd", 0.02f);

  // Pitch PID parameters
  this->declare_parameter<float>("pitch.kp", 0.04f);
  this->declare_parameter<float>("pitch.ki", 0.001f);
  this->declare_parameter<float>("pitch.kd", 0.02f);

  control_rate_hz_ = this->get_parameter("control_rate_hz").as_double();
  deadband_px_ = this->get_parameter("deadband_px").as_double();
  min_angle_ = this->get_parameter("min_angle").as_double();
  max_angle_ = this->get_parameter("max_angle").as_double();

  float yaw_kp = this->get_parameter("yaw.kp").as_double();
  float yaw_ki = this->get_parameter("yaw.ki").as_double();
  float yaw_kd = this->get_parameter("yaw.kd").as_double();

  float pitch_kp = this->get_parameter("pitch.kp").as_double();
  float pitch_ki = this->get_parameter("pitch.ki").as_double();
  float pitch_kd = this->get_parameter("pitch.kd").as_double();

  // Create PID controllers
  pid_yaw_ = std::make_unique<PIDController>(yaw_kp, yaw_ki, yaw_kd, deadband_px_,
                                             -(max_angle_ - min_angle_) / 2.0f,
                                             (max_angle_ - min_angle_) / 2.0f);
  pid_pitch_ = std::make_unique<PIDController>(pitch_kp, pitch_ki, pitch_kd, deadband_px_,
                                               -(max_angle_ - min_angle_) / 2.0f,
                                               (max_angle_ - min_angle_) / 2.0f);

  // Create subscriptions
  pixel_error_sub_ =
    this->create_subscription<geometry_msgs::msg::Vector3>(
      "/pixel_error",
      rclcpp::QoS(5),
      std::bind(&VisualServoNode::OnPixelError, this, std::placeholders::_1));

  servo_state_sub_ =
    this->create_subscription<sensor_msgs::msg::JointState>(
      "/servo_state",
      rclcpp::QoS(10),
      std::bind(&VisualServoNode::OnServoState, this, std::placeholders::_1));

  // Create publisher
  servo_cmd_pub_ =
    this->create_publisher<sensor_msgs::msg::JointState>("/servo_cmd", rclcpp::QoS(5));

  // Create control timer
  int period_ms = static_cast<int>(1000.0f / control_rate_hz_);
  control_timer_ =
    this->create_wall_timer(
      std::chrono::milliseconds(period_ms),
      std::bind(&VisualServoNode::OnControlTimer, this));

  last_control_time_ = this->now();

  RCLCPP_INFO(this->get_logger(),
              "VisualServoNode initialized: rate=%.1f Hz, yaw(Kp=%.3f,Ki=%.4f,Kd=%.3f), pitch(Kp=%.3f,Ki=%.4f,Kd=%.3f)",
              control_rate_hz_, yaw_kp, yaw_ki, yaw_kd, pitch_kp, pitch_ki, pitch_kd);
}

void VisualServoNode::OnPixelError(const geometry_msgs::msg::Vector3::SharedPtr msg) {
  pixel_error_x_ = msg->x;
  pixel_error_y_ = msg->y;
}

void VisualServoNode::OnServoState(const sensor_msgs::msg::JointState::SharedPtr msg) {
  // Update current servo positions from feedback
  for (size_t i = 0; i < msg->name.size(); ++i) {
    if (msg->name[i] == "yaw" && i < msg->position.size()) {
      current_yaw_ = msg->position[i] * 180.0f / M_PI;  // Convert from radians
    } else if (msg->name[i] == "pitch" && i < msg->position.size()) {
      current_pitch_ = msg->position[i] * 180.0f / M_PI;
    }
  }
}

void VisualServoNode::OnControlTimer() {
  // Calculate dt
  auto now = this->now();
  float dt = (now - last_control_time_).seconds();
  last_control_time_ = now;

  if (dt <= 0.0f) {
    return;
  }

  // Get errors (note: negative because pixel error convention)
  float error_yaw = -pixel_error_x_;    // Right error -> positive yaw
  float error_pitch = -pixel_error_y_;  // Down error -> positive pitch

  // Update PID controllers
  float delta_yaw = pid_yaw_->Update(error_yaw, dt);
  float delta_pitch = pid_pitch_->Update(error_pitch, dt);

  // Update desired angles
  float new_yaw = current_yaw_ + delta_yaw;
  float new_pitch = current_pitch_ + delta_pitch;

  // Clamp to mechanical limits
  new_yaw = std::clamp(new_yaw, min_angle_, max_angle_);
  new_pitch = std::clamp(new_pitch, min_angle_, max_angle_);

  // Publish servo command
  auto cmd = sensor_msgs::msg::JointState();
  cmd.header.stamp = now;
  cmd.name = {"yaw", "pitch", "s2", "s3"};
  cmd.position = {
    new_yaw * M_PI / 180.0f,      // Convert to radians
    new_pitch * M_PI / 180.0f,
    0.0f,
    0.0f
  };
  servo_cmd_pub_->publish(cmd);

  RCLCPP_DEBUG(this->get_logger(),
               "PID: error=(%.1f, %.1f), delta=(%.2f, %.2f) -> angle=(%.1f, %.1f)",
               error_yaw, error_pitch, delta_yaw, delta_pitch, new_yaw, new_pitch);
}

}  // namespace visual_servo
