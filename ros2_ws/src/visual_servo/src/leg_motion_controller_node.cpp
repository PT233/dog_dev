#include <algorithm>
#include <array>
#include <cmath>

#include "shared/servo_names.hpp"
#include "visual_servo/leg_motion_controller_node.hpp"

namespace visual_servo
{

namespace
{

// 当前机器人使用 4 路腿舵机，名称顺序由 shared/servo_names.hpp 统一定义。
constexpr size_t kLegCount = project_shared::kServoCount;

}  // namespace

LegMotionControllerNode::LegMotionControllerNode(const rclcpp::NodeOptions & options)
: rclcpp::Node("leg_motion_node", options)
{
  // 运动控制参数：YAML 中可调，默认值保证舵机先在保守范围内运动。
  this->declare_parameter<float>("controller.control_rate_hz", 30.0f);
  this->declare_parameter<float>("controller.deadband_pixels", 5.0f);
  this->declare_parameter<float>("servo.min_angle_deg", 10.0f);
  this->declare_parameter<float>("servo.max_angle_deg", 170.0f);
  this->declare_parameter<float>("servo.neutral_angle_deg", 90.0f);
  this->declare_parameter<float>("gait.stride_amplitude_max_deg", 20.0f);
  this->declare_parameter<float>("gait.turn_bias_max_deg", 12.0f);
  this->declare_parameter<float>("gait.frequency_hz", 2.0f);
  this->declare_parameter<float>("target.timeout_sec", 0.5f);

  // 转向 PID：主要使用水平像素误差，输出左右腿的角度偏置。
  this->declare_parameter<float>("turn_controller.proportional_gain", 0.05f);
  this->declare_parameter<float>("turn_controller.integral_gain", 0.001f);
  this->declare_parameter<float>("turn_controller.derivative_gain", 0.02f);

  // 前进 PID：主要使用垂直像素误差，输出步态摆幅。
  this->declare_parameter<float>("forward_controller.proportional_gain", 0.04f);
  this->declare_parameter<float>("forward_controller.integral_gain", 0.001f);
  this->declare_parameter<float>("forward_controller.derivative_gain", 0.02f);

  control_rate_hz_ = this->get_parameter("controller.control_rate_hz").as_double();
  deadband_pixels_ = this->get_parameter("controller.deadband_pixels").as_double();
  min_angle_deg_ = this->get_parameter("servo.min_angle_deg").as_double();
  max_angle_deg_ = this->get_parameter("servo.max_angle_deg").as_double();
  neutral_angle_deg_ = this->get_parameter("servo.neutral_angle_deg").as_double();
  stride_amplitude_max_deg_ = this->get_parameter("gait.stride_amplitude_max_deg").as_double();
  turn_bias_max_deg_ = this->get_parameter("gait.turn_bias_max_deg").as_double();
  gait_frequency_hz_ = this->get_parameter("gait.frequency_hz").as_double();
  target_timeout_sec_ = this->get_parameter("target.timeout_sec").as_double();

  float turn_proportional_gain =
    this->get_parameter("turn_controller.proportional_gain").as_double();
  float turn_integral_gain = this->get_parameter("turn_controller.integral_gain").as_double();
  float turn_derivative_gain = this->get_parameter("turn_controller.derivative_gain").as_double();

  float forward_proportional_gain =
    this->get_parameter("forward_controller.proportional_gain").as_double();
  float forward_integral_gain =
    this->get_parameter("forward_controller.integral_gain").as_double();
  float forward_derivative_gain =
    this->get_parameter("forward_controller.derivative_gain").as_double();

  // 两个 PID 输出直接限幅到允许的角度增量，避免后续步态叠加过大。
  turn_controller_ = std::make_unique<PidController>(
    turn_proportional_gain, turn_integral_gain, turn_derivative_gain, deadband_pixels_,
    -turn_bias_max_deg_, turn_bias_max_deg_);
  forward_controller_ = std::make_unique<PidController>(
    forward_proportional_gain, forward_integral_gain, forward_derivative_gain,
    deadband_pixels_, -stride_amplitude_max_deg_, stride_amplitude_max_deg_);

  // pixel_error 来自 behavior_node，代表目标相对画面中心的偏移。
  pixel_error_subscription_ =
    this->create_subscription<geometry_msgs::msg::Vector3>(
      "~/input/pixel_error",
      rclcpp::QoS(5),
      std::bind(&LegMotionControllerNode::pixel_error_callback, this, std::placeholders::_1));

  servo_state_subscription_ =
    this->create_subscription<sensor_msgs::msg::JointState>(
      "~/input/servo_state",
      rclcpp::QoS(10),
      std::bind(&LegMotionControllerNode::servo_state_callback, this, std::placeholders::_1));

  // 输出给 uart_bridge 的关节目标，position 单位必须是弧度。
  servo_command_pub_ =
    this->create_publisher<sensor_msgs::msg::JointState>("~/output/servo_command", rclcpp::QoS(5));

  // 固定周期控制环，dt 仍使用 ROS 时间实测，降低定时器抖动影响。
  int period_milliseconds = static_cast<int>(1000.0f / control_rate_hz_);
  control_timer_ =
    this->create_wall_timer(
      std::chrono::milliseconds(period_milliseconds),
      std::bind(&LegMotionControllerNode::control_timer_callback, this));

  last_control_time_ = this->now();
  last_pixel_error_time_ = last_control_time_;

  RCLCPP_INFO(this->get_logger(),
              "LegMotionController initialized: rate=%.1f Hz, neutral=%.1f deg, "
              "stride_max=%.1f deg, turn_max=%.1f deg, gait=%.2f Hz, "
              "turn(Kp=%.3f,Ki=%.4f,Kd=%.3f), forward(Kp=%.3f,Ki=%.4f,Kd=%.3f)",
              control_rate_hz_, neutral_angle_deg_, stride_amplitude_max_deg_,
              turn_bias_max_deg_, gait_frequency_hz_,
              turn_proportional_gain, turn_integral_gain, turn_derivative_gain,
              forward_proportional_gain, forward_integral_gain, forward_derivative_gain);
}

void LegMotionControllerNode::pixel_error_callback(
  const geometry_msgs::msg::Vector3::SharedPtr pixel_error_msg)
{
  // 只缓存最新像素误差，控制环定时读取；z 轴当前未使用。
  pixel_error_x_ = pixel_error_msg->x;
  pixel_error_y_ = pixel_error_msg->y;
  has_pixel_error_ = true;
  last_pixel_error_time_ = this->now();
}

void LegMotionControllerNode::servo_state_callback(
  const sensor_msgs::msg::JointState::SharedPtr servo_state_msg)
{
  // 将反馈角从 ROS JointState 的弧度转换为内部使用的度。
  for (size_t i = 0; i < servo_state_msg->name.size(); ++i) {
    const int servo_id = project_shared::servo_name_to_id(servo_state_msg->name[i]);
    if (servo_id >= 0 && i < servo_state_msg->position.size()) {
      current_leg_angles_deg_[static_cast<size_t>(servo_id)] =
        servo_state_msg->position[i] * 180.0f / static_cast<float>(M_PI);
    }
  }
}

void LegMotionControllerNode::control_timer_callback()
{
  auto now = this->now();
  float delta_time_sec = (now - last_control_time_).seconds();
  last_control_time_ = now;

  if (delta_time_sec <= 0.0f) {
    return;
  }

  const bool target_fresh =
    has_pixel_error_ && ((now - last_pixel_error_time_).seconds() <= target_timeout_sec_);

  // 目标超时后输出回到中位，避免旧视觉误差继续驱动机器人运动。
  float turn_bias = 0.0f;
  float stride_command = 0.0f;

  if (target_fresh) {
    // 固定摄像头下：
    //   error_x > 0 代表目标在画面右侧，需要右转，故 turn_error 取负
    //   error_y < 0 代表目标偏上，可视为需要向前逼近，故 forward_error 取负号
    const float turn_error = -pixel_error_x_;
    const float forward_error = -pixel_error_y_;

    turn_bias = turn_controller_->update(turn_error, delta_time_sec);
    stride_command = forward_controller_->update(forward_error, delta_time_sec);
  } else {
    turn_controller_->reset();
    forward_controller_->reset();
    gait_phase_rad_ = 0.0f;
  }

  const float stride_amplitude =
    std::clamp(std::abs(stride_command), 0.0f, stride_amplitude_max_deg_);
  const float stride_direction = (stride_command >= 0.0f) ? 1.0f : -1.0f;
  turn_bias = std::clamp(turn_bias, -turn_bias_max_deg_, turn_bias_max_deg_);

  if (target_fresh && (stride_amplitude > 0.1f || std::abs(turn_bias) > 0.1f)) {
    gait_phase_rad_ += 2.0f * static_cast<float>(M_PI) * gait_frequency_hz_ * delta_time_sec;
    if (gait_phase_rad_ > 2.0f * static_cast<float>(M_PI)) {
      gait_phase_rad_ = std::fmod(gait_phase_rad_, 2.0f * static_cast<float>(M_PI));
    }
  }

  const float first_diagonal_offset =
    stride_direction * stride_amplitude * std::sin(gait_phase_rad_);
  const float second_diagonal_offset = -first_diagonal_offset;

  // 简化对角步态：前左/后右同相，前右/后左反相；turn_bias 叠加左右差速效果。
  std::array<float, kLegCount> target_angles = {
    neutral_angle_deg_ + first_diagonal_offset + turn_bias,    // front_left
    neutral_angle_deg_ + second_diagonal_offset - turn_bias,   // front_right
    neutral_angle_deg_ + second_diagonal_offset + turn_bias,   // rear_left
    neutral_angle_deg_ + first_diagonal_offset - turn_bias     // rear_right
  };

  for (float & angle : target_angles) {
    angle = std::clamp(angle, min_angle_deg_, max_angle_deg_);
  }

  // 发布舵机目标；下游 uart_bridge 会把弧度转成 STM32 协议的度 ×10。
  auto servo_command = sensor_msgs::msg::JointState();
  servo_command.header.stamp = now;
  servo_command.name.assign(project_shared::kServoNames.begin(), project_shared::kServoNames.end());
  servo_command.position.reserve(kLegCount);
  for (float angle_deg : target_angles) {
    servo_command.position.push_back(angle_deg * static_cast<float>(M_PI) / 180.0f);
  }
  servo_command_pub_->publish(servo_command);

  RCLCPP_DEBUG(this->get_logger(),
               "Leg motion: target=%s error=(%.1f, %.1f) stride=%.2f turn=%.2f "
               "angles=(%.1f, %.1f, %.1f, %.1f)",
               target_fresh ? "locked" : "lost",
               pixel_error_x_, pixel_error_y_, stride_command, turn_bias,
               target_angles[0], target_angles[1], target_angles[2], target_angles[3]);
}

}  // namespace visual_servo
