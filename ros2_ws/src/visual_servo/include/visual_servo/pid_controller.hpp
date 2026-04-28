#pragma once

// 单轴 PID 控制器
// 用于固定摄像头的视觉引导腿部控制，将像素误差（像素）转换为
// 步态幅度或转向偏置（度）。
// 包含三个保护机制：死区过滤、积分饱和（防 windup）、输出限幅。

namespace visual_servo {

// 单轴离散时间 PID 控制器
// 每次调用 Update(error, dt) 返回本次输出量（角度增量，单位：度）
class PIDController {
public:
  // kp/ki/kd: 比例/积分/微分增益
  // deadband: 死区大小（像素），误差绝对值小于此值时视为零误差
  // output_min/max: 输出限幅（度）
  PIDController(float kp = 0.0f, float ki = 0.0f, float kd = 0.0f,
                float deadband = 0.0f, float output_min = -180.0f,
                float output_max = 180.0f);

  // 输入本次误差和时间步长（秒），返回控制量（角度增量，度）
  float Update(float error, float dt);

  // 清零积分项、上次误差、first_call 标志
  void Reset();

  // 运行时动态调整参数（通过 ros2 param set 触发）
  void SetGains(float kp, float ki, float kd);
  void SetLimits(float min_out, float max_out);
  void SetDeadband(float db);

private:
  float kp_, ki_, kd_;
  float deadband_;
  float output_min_, output_max_;

  float integral_;      // 积分累积量（单位：度）
  float prev_error_;    // 上一帧误差（用于微分计算）
  bool first_call_;     // 首次调用标志，跳过微分（因 prev_error_ 无意义）
};

}  // namespace visual_servo
