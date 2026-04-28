#include "visual_servo/pid_controller.hpp"
#include <algorithm>
#include <cmath>

namespace visual_servo {

PIDController::PIDController(float kp, float ki, float kd, float deadband,
                             float output_min, float output_max)
    : kp_(kp), ki_(ki), kd_(kd), deadband_(deadband),
      output_min_(output_min), output_max_(output_max),
      integral_(0.0f), prev_error_(0.0f), first_call_(true) {}

float PIDController::Update(float error, float dt) {
  // 死区：误差绝对值小于 deadband_ 时强制归零，防止目标在画面中心附近时舵机持续微抖
  if (std::abs(error) < deadband_) {
    error = 0.0f;
  }

  float output = 0.0f;

  // 比例项
  output += kp_ * error;

  // 积分项（仅在 dt > 0 时更新，防止时钟抖动导致的除零或异常累积）
  if (dt > 0.0f) {
    integral_ += ki_ * error * dt;
    // 积分饱和保护：限制在 ±output_max/2，防止目标长时间偏离时积分无限累积
    // 导致目标回归后出现大幅超调
    integral_ = std::clamp(integral_, output_min_ / 2.0f, output_max_ / 2.0f);
    output += integral_;
  }

  // 微分项（首次调用跳过：prev_error_ 未初始化，求导无意义）
  if (!first_call_ && dt > 0.0f) {
    float error_rate = (error - prev_error_) / dt;
    output += kd_ * error_rate;
  }

  first_call_ = false;
  prev_error_ = error;

  // 输出限幅：保证指令不超出舵机机械极限（10°~170°）
  output = std::clamp(output, output_min_, output_max_);

  return output;
}

void PIDController::Reset() {
  integral_ = 0.0f;
  prev_error_ = 0.0f;
  first_call_ = true;
}

void PIDController::SetGains(float kp, float ki, float kd) {
  kp_ = kp;
  ki_ = ki;
  kd_ = kd;
}

void PIDController::SetLimits(float min_out, float max_out) {
  output_min_ = min_out;
  output_max_ = max_out;
}

void PIDController::SetDeadband(float db) {
  deadband_ = db;
}

}  // namespace visual_servo
