// 单元测试：visual_servo/PidController
//
// 编译（在 tests/ 目录下执行）：g++ test_pid_controller.cpp -I../ros2_ws/src/visual_servo/include
// ../ros2_ws/src/visual_servo/src/pid_controller.cpp -o test_pid_controller && ./test_pid_controller

#include <cassert>
#include <cmath>
#include <cstdio>
#include "visual_servo/pid_controller.hpp"

// 浮点容差比较
static bool is_near(float value, float expected_value, float epsilon = 0.01f) {
    return std::fabs(value - expected_value) < epsilon;
}

// 测试 1：纯比例控制（Ki=0, Kd=0）
// 预期：output = Kp × error
static void test_proportional() {
    visual_servo::PidController pid_controller(
        /*proportional_gain=*/0.1f, /*integral_gain=*/0.0f, /*derivative_gain=*/0.0f);
    float output = pid_controller.update(/*error=*/100.0f, /*delta_time_sec=*/0.033f);
    assert(is_near(output, 10.0f));  // 0.1 × 100 = 10

    output = pid_controller.update(-50.0f, 0.033f);
    assert(is_near(output, -5.0f));  // 0.1 × (-50) = -5
    printf("PASS test_proportional\n");
}

// 测试 2：死区过滤（|error| < deadband → output = 0）
// 目的：防止目标在画面中心附近时舵机持续微抖
static void test_deadband_suppresses_small_error() {
    visual_servo::PidController pid_controller(1.0f, 0.0f, 0.0f, /*deadband_pixels=*/5.0f);

    float output = pid_controller.update(3.0f, 0.033f);   // 3 < 5，落在死区内
    assert(is_near(output, 0.0f));

    output = pid_controller.update(-4.9f, 0.033f);         // -4.9，绝对值仍在死区内
    assert(is_near(output, 0.0f));

    output = pid_controller.update(10.0f, 0.033f);         // 10 > 5，超出死区
    assert(is_near(output, 10.0f));              // Kp=1 × error=10
    printf("PASS test_deadband_suppresses_small_error\n");
}

// 测试 3：积分项随时间累积
// 预期：每帧 integral += Ki × error × dt
static void test_integral_accumulation() {
    visual_servo::PidController pid_controller(0.0f, /*integral_gain=*/1.0f, 0.0f);
    float delta_time_sec = 0.1f;

    // 第 1 帧：integral = 0 + 1.0×10×0.1 = 1.0，output = 1.0
    pid_controller.update(10.0f, delta_time_sec);

    // 第 2 帧：integral = 1.0 + 1.0×10×0.1 = 2.0，output = 2.0
    float output = pid_controller.update(10.0f, delta_time_sec);
    assert(is_near(output, 2.0f));
    printf("PASS test_integral_accumulation\n");
}

// 测试 4：输出限幅（output ∈ [output_min, output_max]）
static void test_output_clamping() {
    visual_servo::PidController pid_controller(10.0f, 0.0f, 0.0f,
                                               /*deadband_pixels=*/0.0f,
                                               /*min_output=*/-50.0f,
                                               /*max_output=*/50.0f);

    float output = pid_controller.update(100.0f, 0.033f);   // 10×100=1000 → 上限 50
    assert(is_near(output, 50.0f));

    output = pid_controller.update(-100.0f, 0.033f);         // 10×(-100)=-1000 → 下限 -50
    assert(is_near(output, -50.0f));
    printf("PASS test_output_clamping\n");
}

// 测试 5：reset() 清除积分和微分历史状态
// 清零后首帧行为应与全新控制器相同
static void test_reset_clears_state() {
    visual_servo::PidController pid_controller(0.0f, /*integral_gain=*/1.0f, 0.0f);

    pid_controller.update(100.0f, 0.1f);  // 积累大量积分
    pid_controller.update(100.0f, 0.1f);

    pid_controller.reset();

    // reset 后积分从 0 重新计算
    float output = pid_controller.update(10.0f, 0.1f);
    assert(is_near(output, 1.0f));  // integral_gain × error × delta_time_sec = 1.0 × 10 × 0.1
    printf("PASS test_reset_clears_state\n");
}

// 测试 6：微分项（需要至少两次调用）
// 第一次调用因 is_first_update_=true 跳过微分，第二次才计算 d_error/delta_time_sec
static void test_derivative_term() {
    visual_servo::PidController pid_controller(0.0f, 0.0f, /*derivative_gain=*/1.0f);
    float delta_time_sec = 0.1f;

    // 首次调用 is_first_update_=true，无微分输出
    float output1 = pid_controller.update(10.0f, delta_time_sec);
    assert(is_near(output1, 0.0f));

    // 第二次：d_error = (20-10)/0.1 = 100，output = Kd × 100 = 100
    float output2 = pid_controller.update(20.0f, delta_time_sec);
    assert(is_near(output2, 100.0f));
    printf("PASS test_derivative_term\n");
}

// 测试 7：零误差时输出为零（P/I/D 三项均为 0）
static void test_zero_error_gives_zero_output() {
    visual_servo::PidController pid_controller(1.0f, 1.0f, 1.0f);
    float output = pid_controller.update(0.0f, 0.1f);
    assert(is_near(output, 0.0f));
    printf("PASS test_zero_error_gives_zero_output\n");
}

// 测试 8：积分饱和保护（integral 不超过 output_max/2）
// 防止目标长时间偏离时积分无限累积，恢复后出现巨大超调
static void test_integral_anti_windup() {
    visual_servo::PidController pid_controller(0.0f, /*integral_gain=*/10.0f, 0.0f,
                                               /*deadband_pixels=*/0.0f,
                                               /*min_output=*/-100.0f,
                                               /*max_output=*/100.0f);
    // 积分饱和上限 = max_output / 2 = 50

    // 持续 100 帧 error=10，积分会尝试超出上限
    for (int i = 0; i < 100; i++) {
        pid_controller.update(10.0f, 0.1f);
    }
    float output = pid_controller.update(10.0f, 0.1f);

    // 输出应被最终钳制在 [-100, 100]
    assert(output <= 100.0f);
    assert(output >= -100.0f);
    printf("PASS test_integral_anti_windup\n");
}

// 测试 9：delta_time_sec <= 0 时跳过积分和微分（防止除零/时钟抖动）
static void test_zero_dt_skips_i_and_d() {
    visual_servo::PidController pid_controller(1.0f, 1.0f, 1.0f);
    pid_controller.update(10.0f, 0.1f);  // 第一帧建立 previous_error

    // delta_time_sec=0 时，积分和微分不更新，只有比例项
    float output = pid_controller.update(10.0f, 0.0f);
    assert(is_near(output, 10.0f));  // 只有 Kp × error = 1.0 × 10
    printf("PASS test_zero_dt_skips_i_and_d\n");
}

int main() {
    printf("=== PidController Unit Tests ===\n");
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
