#pragma once

namespace visual_servo {

class PIDController {
public:
  PIDController(float kp = 0.0f, float ki = 0.0f, float kd = 0.0f,
                float deadband = 0.0f, float output_min = -180.0f,
                float output_max = 180.0f);

  // Update controller with new error and delta time (seconds)
  float Update(float error, float dt);

  // Reset internal state
  void Reset();

  // Setters
  void SetGains(float kp, float ki, float kd);
  void SetLimits(float min_out, float max_out);
  void SetDeadband(float db);

private:
  float kp_, ki_, kd_;
  float deadband_;
  float output_min_, output_max_;

  float integral_;
  float prev_error_;
  bool first_call_;
};

}  // namespace visual_servo
