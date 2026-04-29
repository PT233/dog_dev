#ifndef __TRAJ_PLANNER_H__
#define __TRAJ_PLANNER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 当前机械结构固定为 4 路腿舵机，和 ROS 侧 shared/servo_names.hpp 的顺序保持一致。 */
#define TRAJ_SERVO_COUNT (4U)

typedef struct {
    float    current_angle;  /* 当前输出角度（度），每 5ms 更新一次 */
    float    target_angle;   /* 本次轨迹最终目标角度（度） */
    float    velocity;       /* 当前瞬时速度（度/秒），用于状态监控 */
    float    max_accel;      /* 梯形速度曲线峰值加速度（度/秒^2） */
    uint32_t start_tick;     /* 轨迹开始时的 HAL_GetTick() */
    uint32_t duration_ms;    /* 本次轨迹总时长，0 表示当前舵机已到位 */
} TrajState;

/* 全局轨迹状态，状态上报任务会从这里复制快照。直接修改会破坏轨迹规划一致性。 */
extern TrajState g_traj_state[TRAJ_SERVO_COUNT];

/* 初始化舵机驱动和所有轨迹状态，默认输出安全中位。 */
void TrajPlanner_Init(void);

/* 设置单路舵机新目标，内部生成梯形速度曲线参数。 */
void Traj_SetTarget(uint8_t id, float target_deg, uint16_t duration_ms);

/* 在临界区复制轨迹状态，供 UART 状态帧打包，避免读到更新一半的数据。 */
void TrajPlanner_CopyStateSnapshot(TrajState out_states[TRAJ_SERVO_COUNT]);

/* 返回轨迹规划任务剩余栈水位，用于运行时诊断栈是否过小。 */
uint32_t TrajPlanner_GetStackHighWaterMark(void);

/* 创建 FreeRTOS 轨迹规划任务。 */
void TrajPlannerTask_Create(void);

#ifdef __cplusplus
}
#endif

#endif /* __TRAJ_PLANNER_H__ */
