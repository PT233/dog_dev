#pragma once

// 视觉引导腿部控制节点
// 固定摄像头不参与转动；视觉链输出的 /pixel_error 用来生成机器人腿部动作。
// 该节点以 30Hz 定时器驱动一个简化步态控制器：
//   /pixel_error (Vector3) → 前进/转向控制量 → 4 路腿舵机 /servo_cmd (JointState, 弧度)
// 同时订阅 /servo_state 获取当前腿舵机角度反馈。

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/vector3.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "visual_servo/pid_controller.hpp"
#include <array>
#include <memory>

namespace visual_servo {

// 视觉引导的腿部控制节点。
// 使用两个 PID 通道分别生成前进幅度和转向偏置，再合成为四条腿的目标角度。
class LegMotionControllerNode : public rclcpp::Node {
public:
  LegMotionControllerNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
  // 订阅：像素误差（behavior_node 发布）
  rclcpp::Subscription<geometry_msgs::msg::Vector3>::SharedPtr pixel_error_sub_;
  // 订阅：腿舵机当前角度反馈（uart_bridge_node 发布，弧度）
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr servo_state_sub_;
  // 发布：4 路腿舵机目标角度命令（uart_bridge_node 订阅，弧度）
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr servo_cmd_pub_;

  // 30Hz 控制定时器
  rclcpp::TimerBase::SharedPtr control_timer_;

  // 最新一帧像素误差（由 OnPixelError 更新，由 OnControlTimer 消费）
  float pixel_error_x_ = 0.0f;
  float pixel_error_y_ = 0.0f;
  std::array<float, 4> current_leg_angles_deg_ = {90.0f, 90.0f, 90.0f, 90.0f};
  bool have_pixel_error_ = false;
  rclcpp::Time last_pixel_error_time_;

  // 控制参数（从 YAML 参数文件加载）
  float deadband_px_ = 5.0f;             // 死区（像素）
  float min_angle_ = 10.0f;              // 舵机角度下限（度）
  float max_angle_ = 170.0f;             // 舵机角度上限（度）
  float neutral_angle_deg_ = 90.0f;      // 腿舵机中立位（度）
  float stride_amplitude_max_deg_ = 20.0f;  // 最大步态摆幅（度）
  float turn_bias_max_deg_ = 12.0f;         // 最大左右差速偏置（度）
  float gait_frequency_hz_ = 2.0f;          // 步态频率（Hz）
  float target_timeout_sec_ = 0.5f;         // 目标丢失超时（秒）
  float control_rate_hz_ = 30.0f;           // 控制环频率
  float gait_phase_rad_ = 0.0f;             // 步态相位

  // 前进和转向各一个独立 PID 控制器
  std::unique_ptr<PIDController> pid_forward_;
  std::unique_ptr<PIDController> pid_turn_;

  // 上一次控制循环的时间戳，用于计算 dt（离散时间步长）
  rclcpp::Time last_control_time_;

  // 回调函数
  void OnPixelError(const geometry_msgs::msg::Vector3::SharedPtr msg);
  // 从 /servo_state 更新当前腿舵机角度（弧度→度）
  void OnServoState(const sensor_msgs::msg::JointState::SharedPtr msg);
  // 30Hz 控制循环：计算 dt → PID 更新 → 合成四腿目标角 → 发布 /servo_cmd
  void OnControlTimer();
};

}  // namespace visual_servo
