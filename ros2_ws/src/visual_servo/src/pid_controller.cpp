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
  // Apply deadband
  if (std::abs(error) < deadband_) {
    error = 0.0f;
  }

  float output = 0.0f;

  // P term
  output += kp_ * error;

  // I term (with saturation protection)
  if (dt > 0.0f) {
    integral_ += ki_ * error * dt;
    // Saturate integral to prevent windup
    integral_ = std::clamp(integral_, output_min_ / 2.0f, output_max_ / 2.0f);
    output += integral_;
  }

  // D term
  if (!first_call_ && dt > 0.0f) {
    float error_rate = (error - prev_error_) / dt;
    output += kd_ * error_rate;
  }

  first_call_ = false;
  prev_error_ = error;

  // Clamp output
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
