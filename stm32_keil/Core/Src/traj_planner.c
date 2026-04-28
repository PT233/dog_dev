// 舵机梯形速度曲线轨迹规划器
// 将目标角度和运动时长分解为每 5ms 的中间角度，驱动 servo_driver 平滑运动。
//
// 梯形速度曲线（Trapezoidal Velocity Profile）：
//   总时间 T，位移 D = target - start
//   加速段（0 ~ t1 = 0.3T）：angle = start + 0.5 × a × t²
//   匀速段（t1 ~ t2 = 0.7T）：angle = start + 匀速段起始位移 + v_max × (t - t1)
//   减速段（t2 ~ T）：       angle = 匀速段末 + v_max × τ - 0.5 × a × τ²（τ = t - t2）
//
//   v_max = D / (T × 0.7)  ←  匀速段占 40% 时间
//   a = v_max / (T × 0.3)  ←  加速段占 30% 时间

#include "traj_planner.h"
#include "servo_driver.h"
#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include <math.h>

// 加速/减速各占总时间的比例（对称梯形，匀速段占 1 - 2×0.3 = 40%）
#define TRAJ_ACCEL_RATIO  0.3f
// 轨迹更新周期（ms），由 FreeRTOS 定时器保证
#define TRAJ_TICK_MS      5U

TrajState g_traj_state[TRAJ_SERVO_COUNT];

/* Per-servo trajectory start angle and peak velocity (private) */
static float s_start_angle[TRAJ_SERVO_COUNT];
static float s_v_max[TRAJ_SERVO_COUNT];

void TrajPlanner_Init(void)
{
    Servo_Init();
    for (uint8_t i = 0; i < TRAJ_SERVO_COUNT; i++) {
        g_traj_state[i].current_angle = SERVO_CENTER_ANGLE_DEG;
        g_traj_state[i].target_angle  = SERVO_CENTER_ANGLE_DEG;
        g_traj_state[i].velocity      = 0.0f;
        g_traj_state[i].max_accel     = 0.0f;
        g_traj_state[i].start_tick    = 0U;
        g_traj_state[i].duration_ms   = 0U;
        s_start_angle[i]              = SERVO_CENTER_ANGLE_DEG;
        s_v_max[i]                    = 0.0f;
    }
}

void Traj_SetTarget(uint8_t id, float target_deg, uint16_t duration_ms)
{
    if (id >= TRAJ_SERVO_COUNT) {
        return;
    }

    float dist = target_deg - g_traj_state[id].current_angle;
    float T    = (float)duration_ms / 1000.0f;

    g_traj_state[id].target_angle = target_deg;
    g_traj_state[id].duration_ms  = duration_ms;
    g_traj_state[id].start_tick   = HAL_GetTick();
    g_traj_state[id].velocity     = 0.0f;
    s_start_angle[id]             = g_traj_state[id].current_angle;

    if (T > 0.001f && fabsf(dist) > 0.01f) {
        // 对称梯形：匀速段占 (1-2r)T，加速段占 rT
        // v_max × (1-2r)T + v_max × rT = D  →  v_max = D / (T × (1-r))
        // 注意：分母是 (1-r) 而非 (1-2r)，因为加速段也有位移贡献（面积=0.5×v×t）
        s_v_max[id]                  = dist / (T * (1.0f - TRAJ_ACCEL_RATIO));
        g_traj_state[id].max_accel   = s_v_max[id] / (T * TRAJ_ACCEL_RATIO);
    } else {
        // 位移极小或时长为 0：立即到位（避免除零和无意义的轨迹规划）
        s_v_max[id]                  = 0.0f;
        g_traj_state[id].max_accel   = 0.0f;
        g_traj_state[id].current_angle = target_deg;
        g_traj_state[id].duration_ms   = 0U;
        Servo_SetAngle(id, target_deg);
    }
}

/* Compute and output the intermediate angle for one servo at current time */
static void Traj_Update(uint8_t id)
{
    TrajState *s = &g_traj_state[id];

    if (s->duration_ms == 0U) {
        return;
    }

    uint32_t elapsed_ms = HAL_GetTick() - s->start_tick;

    /* Trajectory finished */
    if (elapsed_ms >= s->duration_ms) {
        s->current_angle = s->target_angle;
        s->velocity      = 0.0f;
        s->duration_ms   = 0U;
        Servo_SetAngle(id, s->target_angle);
        return;
    }

    float T   = (float)s->duration_ms / 1000.0f;
    float t   = (float)elapsed_ms     / 1000.0f;
    float t1  = T * TRAJ_ACCEL_RATIO;
    float t2  = T * (1.0f - TRAJ_ACCEL_RATIO);
    float v   = s_v_max[id];
    float a   = s->max_accel;
    float angle;
    float vel;

    if (t <= t1) {
        // 加速段：匀加速运动，v = a×t，x = 0.5×a×t²
        vel   = a * t;
        angle = s_start_angle[id] + 0.5f * a * t * t;
    } else if (t <= t2) {
        // 匀速段：速度恒为 v_max，位移线性增长
        vel   = v;
        angle = s_start_angle[id] + 0.5f * v * t1 + v * (t - t1);
    } else {
        // 减速段：从 t2 开始减速，τ = t - t2 为段内时间
        // 位移 = 加速段末位移 + 匀速段位移 + 减速段位移
        float tau        = t - t2;
        float dist_prior = 0.5f * v * t1 + v * (t2 - t1);
        vel   = v - a * tau;
        angle = s_start_angle[id] + dist_prior + v * tau - 0.5f * a * tau * tau;
    }

    s->current_angle = angle;
    s->velocity      = vel;
    Servo_SetAngle(id, angle);
}

static void Task_Traj_Planner(void *arg)
{
    (void)arg;
    for (;;) {
        for (uint8_t i = 0; i < TRAJ_SERVO_COUNT; i++) {
            Traj_Update(i);
        }
        osDelay(TRAJ_TICK_MS);
    }
}

void TrajPlannerTask_Create(void)
{
    static const osThreadAttr_t attr = {
        .name       = "TrajPlan",
        .stack_size = 256U * 4U,
        .priority   = (osPriority_t)osPriorityAboveNormal,
    };
    osThreadNew(Task_Traj_Planner, NULL, &attr);
}
