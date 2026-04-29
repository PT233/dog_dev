#include "visual_servo/pid_controller.hpp"
#include <algorithm>
#include <cmath>

namespace visual_servo
{

PidController::PidController(
  float proportional_gain, float integral_gain,
  float derivative_gain, float deadband_pixels,
  float min_output, float max_output)
: proportional_gain_(proportional_gain),
  integral_gain_(integral_gain),
  derivative_gain_(derivative_gain),
  deadband_pixels_(deadband_pixels),
  min_output_(min_output),
  max_output_(max_output),
  integral_(0.0f),
  previous_error_(0.0f),
  is_first_update_(true) {}

float PidController::update(float error, float delta_time_sec)
{
  // 死区：误差绝对值小于 deadband_pixels_ 时强制归零，防止目标在画面中心附近时舵机持续微抖
  if (std::abs(error) < deadband_pixels_) {
    error = 0.0f;
  }

  float output = 0.0f;

  // 比例项
  output += proportional_gain_ * error;

  // 积分项（仅在 delta_time_sec > 0 时更新，防止时钟抖动导致的除零或异常累积）
  if (delta_time_sec > 0.0f) {
    integral_ += integral_gain_ * error * delta_time_sec;
    // 积分饱和保护：限制在 ±max_output_/2，防止目标长时间偏离时积分无限累积
    // 导致目标回归后出现大幅超调
    integral_ = std::clamp(integral_, min_output_ / 2.0f, max_output_ / 2.0f);
    output += integral_;
  }

  // 微分项（首次调用跳过：previous_error_ 未初始化，求导无意义）
  if (!is_first_update_ && delta_time_sec > 0.0f) {
    float error_rate = (error - previous_error_) / delta_time_sec;
    output += derivative_gain_ * error_rate;
  }

  is_first_update_ = false;
  previous_error_ = error;

  // 输出限幅：保证指令不超出舵机机械极限（10°~170°）
  output = std::clamp(output, min_output_, max_output_);

  return output;
}

void PidController::reset()
{
  integral_ = 0.0f;
  previous_error_ = 0.0f;
  is_first_update_ = true;
}

void PidController::set_gains(
  float proportional_gain, float integral_gain, float derivative_gain)
{
  proportional_gain_ = proportional_gain;
  integral_gain_ = integral_gain;
  derivative_gain_ = derivative_gain;
}

void PidController::set_limits(float min_output, float max_output)
{
  min_output_ = min_output;
  max_output_ = max_output;
}

void PidController::set_deadband(float deadband_pixels)
{
  deadband_pixels_ = deadband_pixels;
}

}  // namespace visual_servo
