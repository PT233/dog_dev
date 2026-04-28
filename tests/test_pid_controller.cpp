// 单元测试：visual_servo/PIDController
//
// 编译（在 tests/ 目录下执行）：
//   g++ test_pid_controller.cpp \
//       -I../ros2_ws/src/visual_servo/include \
//       ../ros2_ws/src/visual_servo/src/pid_controller.cpp \
//       -o test_pid_controller && ./test_pid_controller

#include <cassert>
#include <cmath>
#include <cstdio>
#include "visual_servo/pid_controller.hpp"

// 浮点容差比较
static bool near_f(float a, float b, float eps = 0.01f) {
    return std::fabs(a - b) < eps;
}

// 测试 1：纯比例控制（Ki=0, Kd=0）
// 预期：output = Kp × error
static void test_proportional() {
    visual_servo::PIDController pid(/*kp=*/0.1f, /*ki=*/0.0f, /*kd=*/0.0f);
    float out = pid.Update(/*error=*/100.0f, /*dt=*/0.033f);
    assert(near_f(out, 10.0f));  // 0.1 × 100 = 10

    out = pid.Update(-50.0f, 0.033f);
    assert(near_f(out, -5.0f));  // 0.1 × (-50) = -5
    printf("PASS test_proportional\n");
}

// 测试 2：死区过滤（|error| < deadband → output = 0）
// 目的：防止目标在画面中心附近时舵机持续微抖
static void test_deadband_suppresses_small_error() {
    visual_servo::PIDController pid(1.0f, 0.0f, 0.0f, /*deadband=*/5.0f);

    float out = pid.Update(3.0f, 0.033f);   // 3 < 5，落在死区内
    assert(near_f(out, 0.0f));

    out = pid.Update(-4.9f, 0.033f);         // -4.9，绝对值仍在死区内
    assert(near_f(out, 0.0f));

    out = pid.Update(10.0f, 0.033f);         // 10 > 5，超出死区
    assert(near_f(out, 10.0f));              // Kp=1 × error=10
    printf("PASS test_deadband_suppresses_small_error\n");
}

// 测试 3：积分项随时间累积
// 预期：每帧 integral += Ki × error × dt
static void test_integral_accumulation() {
    visual_servo::PIDController pid(0.0f, /*ki=*/1.0f, 0.0f);
    float dt = 0.1f;

    // 第 1 帧：integral = 0 + 1.0×10×0.1 = 1.0，output = 1.0
    pid.Update(10.0f, dt);

    // 第 2 帧：integral = 1.0 + 1.0×10×0.1 = 2.0，output = 2.0
    float out = pid.Update(10.0f, dt);
    assert(near_f(out, 2.0f));
    printf("PASS test_integral_accumulation\n");
}

// 测试 4：输出限幅（output ∈ [output_min, output_max]）
static void test_output_clamping() {
    visual_servo::PIDController pid(10.0f, 0.0f, 0.0f,
                                    /*deadband=*/0.0f,
                                    /*output_min=*/-50.0f,
                                    /*output_max=*/50.0f);

    float out = pid.Update(100.0f, 0.033f);   // 10×100=1000 → 上限 50
    assert(near_f(out, 50.0f));

    out = pid.Update(-100.0f, 0.033f);         // 10×(-100)=-1000 → 下限 -50
    assert(near_f(out, -50.0f));
    printf("PASS test_output_clamping\n");
}

// 测试 5：Reset() 清除积分和微分历史状态
// 清零后首帧行为应与全新控制器相同
static void test_reset_clears_state() {
    visual_servo::PIDController pid(0.0f, /*ki=*/1.0f, 0.0f);

    pid.Update(100.0f, 0.1f);  // 积累大量积分
    pid.Update(100.0f, 0.1f);

    pid.Reset();

    // Reset 后积分从 0 重新计算
    float out = pid.Update(10.0f, 0.1f);
    assert(near_f(out, 1.0f));  // ki × error × dt = 1.0 × 10 × 0.1
    printf("PASS test_reset_clears_state\n");
}

// 测试 6：微分项（需要至少两次调用）
// 第一次调用因 first_call_=true 跳过微分，第二次才计算 d_error/dt
static void test_derivative_term() {
    visual_servo::PIDController pid(0.0f, 0.0f, /*kd=*/1.0f);
    float dt = 0.1f;

    // 首次调用 first_call_=true，无微分输出
    float out1 = pid.Update(10.0f, dt);
    assert(near_f(out1, 0.0f));

    // 第二次：d_error = (20-10)/0.1 = 100，output = Kd × 100 = 100
    float out2 = pid.Update(20.0f, dt);
    assert(near_f(out2, 100.0f));
    printf("PASS test_derivative_term\n");
}

// 测试 7：零误差时输出为零（P/I/D 三项均为 0）
static void test_zero_error_gives_zero_output() {
    visual_servo::PIDController pid(1.0f, 1.0f, 1.0f);
    float out = pid.Update(0.0f, 0.1f);
    assert(near_f(out, 0.0f));
    printf("PASS test_zero_error_gives_zero_output\n");
}

// 测试 8：积分饱和保护（integral 不超过 output_max/2）
// 防止目标长时间偏离时积分无限累积，恢复后出现巨大超调
static void test_integral_anti_windup() {
    visual_servo::PIDController pid(0.0f, /*ki=*/10.0f, 0.0f,
                                    /*deadband=*/0.0f,
                                    /*output_min=*/-100.0f,
                                    /*output_max=*/100.0f);
    // 积分饱和上限 = output_max / 2 = 50

    // 持续 100 帧 error=10，积分会尝试超出上限
    for (int i = 0; i < 100; i++) {
        pid.Update(10.0f, 0.1f);
    }
    float out = pid.Update(10.0f, 0.1f);

    // 输出应被最终钳制在 [-100, 100]
    assert(out <= 100.0f);
    assert(out >= -100.0f);
    printf("PASS test_integral_anti_windup\n");
}

// 测试 9：dt <= 0 时跳过积分和微分（防止除零/时钟抖动）
static void test_zero_dt_skips_i_and_d() {
    visual_servo::PIDController pid(1.0f, 1.0f, 1.0f);
    pid.Update(10.0f, 0.1f);  // 第一帧建立 prev_error

    // dt=0 时，积分和微分不更新，只有比例项
    float out = pid.Update(10.0f, 0.0f);
    assert(near_f(out, 10.0f));  // 只有 Kp × error = 1.0 × 10
    printf("PASS test_zero_dt_skips_i_and_d\n");
}

int main() {
    printf("=== PIDController Unit Tests ===\n");
    test_proportional();
    test_deadband_suppresses_small_error();
    test_integral_accumulation();
    test_output_clamping();
    test_reset_clears_state();
    test_derivative_term();
    test_zero_error_gives_zero_output();
    test_integral_anti_windup();
    test_zero_dt_skips_i_and_d();
    printf("=== All tests PASSED ===\n");
    return 0;
}
