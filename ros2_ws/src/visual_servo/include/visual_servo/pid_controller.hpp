#ifndef VISUAL_SERVO__PID_CONTROLLER_HPP_
#define VISUAL_SERVO__PID_CONTROLLER_HPP_

// 单轴 PID 控制器
// 用于固定摄像头的视觉引导腿部控制，将像素误差（像素）转换为
// 步态幅度或转向偏置（度）。
// 包含三个保护机制：死区过滤、积分饱和（防 windup）、输出限幅。

namespace visual_servo
{

// 单轴离散时间 PID 控制器
// 每次调用 update(error, delta_time_sec) 返回本次输出量（角度增量，单位：度）
class PidController {
public:
  // proportional_gain/integral_gain/derivative_gain: 比例/积分/微分增益
  // deadband_pixels: 死区大小（像素），误差绝对值小于此值时视为零误差
  // min_output/max_output: 输出限幅（度）
  PidController(
    float proportional_gain = 0.0f, float integral_gain = 0.0f,
    float derivative_gain = 0.0f, float deadband_pixels = 0.0f,
    float min_output = -180.0f, float max_output = 180.0f);

  // 输入本次误差和时间步长（秒），返回控制量（角度增量，度）
  float update(float error, float delta_time_sec);

  // 清零积分项、上次误差、is_first_update_ 标志
  void reset();

  // 运行时动态调整参数（通过 ros2 param set 触发）
  void set_gains(float proportional_gain, float integral_gain, float derivative_gain);
  void set_limits(float min_output, float max_output);
  void set_deadband(float deadband_pixels);

private:
  float proportional_gain_;
  float integral_gain_;
  float derivative_gain_;
  float deadband_pixels_;
  float min_output_;
  float max_output_;

  float integral_;         // 积分累积量（单位：度）
  float previous_error_;   // 上一帧误差（用于微分计算）
  bool is_first_update_;   // 首次调用标志，跳过微分（因 previous_error_ 无意义）
};

}  // namespace visual_servo

#endif  // VISUAL_SERVO__PID_CONTROLLER_HPP_
