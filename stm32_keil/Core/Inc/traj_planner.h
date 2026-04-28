#ifndef __TRAJ_PLANNER_H__
#define __TRAJ_PLANNER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define TRAJ_SERVO_COUNT (4U)

typedef struct {
    float    current_angle;  /* current output angle (deg) */
    float    target_angle;   /* final destination (deg) */
    float    velocity;       /* instantaneous velocity (deg/s) */
    float    max_accel;      /* peak acceleration (deg/s^2) */
    uint32_t start_tick;     /* HAL_GetTick() at trajectory start */
    uint32_t duration_ms;    /* total trajectory duration (ms) */
} TrajState;

/* Global trajectory states, readable by other modules (e.g. Status_TX) */
extern TrajState g_traj_state[TRAJ_SERVO_COUNT];

void TrajPlanner_Init(void);
void Traj_SetTarget(uint8_t id, float target_deg, uint16_t duration_ms);
void TrajPlanner_CopyStateSnapshot(TrajState out_states[TRAJ_SERVO_COUNT]);
uint32_t TrajPlanner_GetStackHighWaterMark(void);
void TrajPlannerTask_Create(void);

#ifdef __cplusplus
}
#endif

#endif /* __TRAJ_PLANNER_H__ */
