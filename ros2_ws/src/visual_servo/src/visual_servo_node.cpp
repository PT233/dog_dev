#include "visual_servo/visual_servo_node.hpp"
#include <array>
#include <cmath>
#include <algorithm>

namespace visual_servo {

namespace {

constexpr size_t kLegCount = 4;
constexpr std::array<const char*, kLegCount> kLegJointNames = {
  "front_left",
  "front_right",
  "rear_left",
  "rear_right"
};

int JointNameToIndex(const std::string& name) {
  for (size_t i = 0; i < kLegJointNames.size(); ++i) {
    if (name == kLegJointNames[i]) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace

LegMotionControllerNode::LegMotionControllerNode(const rclcpp::NodeOptions& options)
    : rclcpp::Node("leg_motion_node", options) {
  // Declare and get parameters
  this->declare_parameter<float>("control_rate_hz", 30.0f);
  this->declare_parameter<float>("deadband_px", 5.0f);
  this->declare_parameter<float>("min_angle", 10.0f);
  this->declare_parameter<float>("max_angle", 170.0f);
  this->declare_parameter<float>("neutral_angle_deg", 90.0f);
  this->declare_parameter<float>("stride_amplitude_max_deg", 20.0f);
  this->declare_parameter<float>("turn_bias_max_deg", 12.0f);
  this->declare_parameter<float>("gait_frequency_hz", 2.0f);
  this->declare_parameter<float>("target_timeout_sec", 0.5f);

  // Turn PID parameters
  this->declare_parameter<float>("turn.kp", 0.05f);
  this->declare_parameter<float>("turn.ki", 0.001f);
  this->declare_parameter<float>("turn.kd", 0.02f);

  // Forward PID parameters
  this->declare_parameter<float>("forward.kp", 0.04f);
  this->declare_parameter<float>("forward.ki", 0.001f);
  this->declare_parameter<float>("forward.kd", 0.02f);

  control_rate_hz_ = this->get_parameter("control_rate_hz").as_double();
  deadband_px_ = this->get_parameter("deadband_px").as_double();
  min_angle_ = this->get_parameter("min_angle").as_double();
  max_angle_ = this->get_parameter("max_angle").as_double();
  neutral_angle_deg_ = this->get_parameter("neutral_angle_deg").as_double();
  stride_amplitude_max_deg_ = this->get_parameter("stride_amplitude_max_deg").as_double();
  turn_bias_max_deg_ = this->get_parameter("turn_bias_max_deg").as_double();
  gait_frequency_hz_ = this->get_parameter("gait_frequency_hz").as_double();
  target_timeout_sec_ = this->get_parameter("target_timeout_sec").as_double();

  float turn_kp = this->get_parameter("turn.kp").as_double();
  float turn_ki = this->get_parameter("turn.ki").as_double();
  float turn_kd = this->get_parameter("turn.kd").as_double();

  float forward_kp = this->get_parameter("forward.kp").as_double();
  float forward_ki = this->get_parameter("forward.ki").as_double();
  float forward_kd = this->get_parameter("forward.kd").as_double();

  // Create PID controllers
  pid_turn_ = std::make_unique<PIDController>(turn_kp, turn_ki, turn_kd, deadband_px_,
                                              -turn_bias_max_deg_, turn_bias_max_deg_);
  pid_forward_ = std::make_unique<PIDController>(forward_kp, forward_ki, forward_kd, deadband_px_,
                                                 -stride_amplitude_max_deg_,
                                                 stride_amplitude_max_deg_);

  // Create subscriptions
  pixel_error_sub_ =
    this->create_subscription<geometry_msgs::msg::Vector3>(
      "/pixel_error",
      rclcpp::QoS(5),
      std::bind(&LegMotionControllerNode::OnPixelError, this, std::placeholders::_1));

  servo_state_sub_ =
    this->create_subscription<sensor_msgs::msg::JointState>(
      "/servo_state",
      rclcpp::QoS(10),
      std::bind(&LegMotionControllerNode::OnServoState, this, std::placeholders::_1));

  // Create publisher
  servo_cmd_pub_ =
    this->create_publisher<sensor_msgs::msg::JointState>("/servo_cmd", rclcpp::QoS(5));

  // Create control timer
  int period_ms = static_cast<int>(1000.0f / control_rate_hz_);
  control_timer_ =
    this->create_wall_timer(
      std::chrono::milliseconds(period_ms),
      std::bind(&LegMotionControllerNode::OnControlTimer, this));

  last_control_time_ = this->now();
  last_pixel_error_time_ = last_control_time_;

  RCLCPP_INFO(this->get_logger(),
              "LegMotionController initialized: rate=%.1f Hz, neutral=%.1f deg, "
              "stride_max=%.1f deg, turn_max=%.1f deg, gait=%.2f Hz, "
              "turn(Kp=%.3f,Ki=%.4f,Kd=%.3f), forward(Kp=%.3f,Ki=%.4f,Kd=%.3f)",
              control_rate_hz_, neutral_angle_deg_, stride_amplitude_max_deg_,
              turn_bias_max_deg_, gait_frequency_hz_,
              turn_kp, turn_ki, turn_kd, forward_kp, forward_ki, forward_kd);
}

void LegMotionControllerNode::OnPixelError(const geometry_msgs::msg::Vector3::SharedPtr msg) {
  pixel_error_x_ = msg->x;
  pixel_error_y_ = msg->y;
  have_pixel_error_ = true;
  last_pixel_error_time_ = this->now();
}

void LegMotionControllerNode::OnServoState(const sensor_msgs::msg::JointState::SharedPtr msg) {
  for (size_t i = 0; i < msg->name.size(); ++i) {
    const int idx = JointNameToIndex(msg->name[i]);
    if (idx >= 0 && i < msg->position.size()) {
      current_leg_angles_deg_[static_cast<size_t>(idx)] =
        msg->position[i] * 180.0f / static_cast<float>(M_PI);
    }
  }
}

void LegMotionControllerNode::OnControlTimer() {
  auto now = this->now();
  float dt = (now - last_control_time_).seconds();
  last_control_time_ = now;

  if (dt <= 0.0f) {
    return;
  }

  const bool target_fresh =
    have_pixel_error_ && ((now - last_pixel_error_time_).seconds() <= target_timeout_sec_);

  float turn_bias = 0.0f;
  float stride_command = 0.0f;

  if (target_fresh) {
    // 固定摄像头下：
    //   error_x > 0 代表目标在画面右侧，需要右转，故 turn_error 取负
    //   error_y < 0 代表目标偏上，可视为需要向前逼近，故 forward_error 取负号
    const float turn_error = -pixel_error_x_;
    const float forward_error = -pixel_error_y_;

    turn_bias = pid_turn_->Update(turn_error, dt);
    stride_command = pid_forward_->Update(forward_error, dt);
  } else {
    pid_turn_->Reset();
    pid_forward_->Reset();
    gait_phase_rad_ = 0.0f;
  }

  const float stride_amplitude =
    std::clamp(std::abs(stride_command), 0.0f, stride_amplitude_max_deg_);
  const float stride_direction = (stride_command >= 0.0f) ? 1.0f : -1.0f;
  turn_bias = std::clamp(turn_bias, -turn_bias_max_deg_, turn_bias_max_deg_);

  if (target_fresh && (stride_amplitude > 0.1f || std::abs(turn_bias) > 0.1f)) {
    gait_phase_rad_ += 2.0f * static_cast<float>(M_PI) * gait_frequency_hz_ * dt;
    if (gait_phase_rad_ > 2.0f * static_cast<float>(M_PI)) {
      gait_phase_rad_ = std::fmod(gait_phase_rad_, 2.0f * static_cast<float>(M_PI));
    }
  }

  const float phase_a = stride_direction * stride_amplitude * std::sin(gait_phase_rad_);
  const float phase_b = -phase_a;

  std::array<float, kLegCount> target_angles = {
    neutral_angle_deg_ + phase_a + turn_bias,  // front_left
    neutral_angle_deg_ + phase_b - turn_bias,  // front_right
    neutral_angle_deg_ + phase_b + turn_bias,  // rear_left
    neutral_angle_deg_ + phase_a - turn_bias   // rear_right
  };

  for (float& angle : target_angles) {
    angle = std::clamp(angle, min_angle_, max_angle_);
  }

  // Publish servo command
  auto cmd = sensor_msgs::msg::JointState();
  cmd.header.stamp = now;
  cmd.name = {
    kLegJointNames[0],
    kLegJointNames[1],
    kLegJointNames[2],
    kLegJointNames[3]
  };
  cmd.position.reserve(kLegCount);
  for (float angle_deg : target_angles) {
    cmd.position.push_back(angle_deg * static_cast<float>(M_PI) / 180.0f);
  }
  servo_cmd_pub_->publish(cmd);

  RCLCPP_DEBUG(this->get_logger(),
               "Leg motion: target=%s error=(%.1f, %.1f) stride=%.2f turn=%.2f "
               "angles=(%.1f, %.1f, %.1f, %.1f)",
               target_fresh ? "locked" : "lost",
               pixel_error_x_, pixel_error_y_, stride_command, turn_bias,
               target_angles[0], target_angles[1], target_angles[2], target_angles[3]);
}

}  // namespace visual_servo
