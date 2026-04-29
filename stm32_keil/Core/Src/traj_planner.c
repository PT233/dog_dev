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
#include "task.h"

#include <math.h>
#include <string.h>

// 加速/减速各占总时间的比例（对称梯形，匀速段占 1 - 2×0.3 = 40%）
#define TRAJ_ACCEL_RATIO  0.3f
// 轨迹更新周期（ms），由 FreeRTOS 定时器保证
#define TRAJ_TICK_MS      5U

TrajState g_traj_state[TRAJ_SERVO_COUNT];

/* 每路舵机轨迹的起点角和峰值速度，仅在规划器内部使用。 */
static float s_start_angle[TRAJ_SERVO_COUNT];
static float s_v_max[TRAJ_SERVO_COUNT];
static volatile uint32_t s_traj_stack_high_water_mark = 0U;

void TrajPlanner_Init(void)
{
    Servo_Init();
    /* 初始化状态与物理输出同步，确保状态上报中的 current_angle 与真实 PWM 一致。 */
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

    /* 以当前输出角作为轨迹起点，允许运动过程中实时改目标。 */
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

void TrajPlanner_CopyStateSnapshot(TrajState out_states[TRAJ_SERVO_COUNT])
{
    if (out_states == NULL) {
        return;
    }

    /* g_traj_state 会被轨迹任务周期更新，复制时短暂关中断保证快照字段自洽。 */
    taskENTER_CRITICAL();
    memcpy(out_states, g_traj_state, sizeof(g_traj_state));
    taskEXIT_CRITICAL();
}

uint32_t TrajPlanner_GetStackHighWaterMark(void)
{
    return s_traj_stack_high_water_mark;
}

/* 根据当前时间计算单路舵机的中间角度并写入 PWM。 */
static void Traj_Update(uint8_t id)
{
    TrajState *s = &g_traj_state[id];

    if (s->duration_ms == 0U) {
        return;
    }

    uint32_t elapsed_ms = HAL_GetTick() - s->start_tick;

    /* 已到达规划时长：强制写最终角，避免浮点累计误差留下尾差。 */
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
    s_traj_stack_high_water_mark = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    for (;;) {
        s_traj_stack_high_water_mark = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
        /* 5ms 周期遍历所有舵机，使状态上报和实际 PWM 都保持平滑连续。 */
        for (uint8_t i = 0; i < TRAJ_SERVO_COUNT; i++) {
            Traj_Update(i);
        }
        osDelay(TRAJ_TICK_MS);
    }
}

void TrajPlannerTask_Create(void)
{
    osThreadId_t handle;
    static const osThreadAttr_t attr = {
        .name       = "TrajPlan",
        .stack_size = 256U * 4U,
        .priority   = (osPriority_t)osPriorityAboveNormal,
    };
    handle = osThreadNew(Task_Traj_Planner, NULL, &attr);
    configASSERT(handle != NULL);
}
