#ifndef __SERVO_DRIVER_H__
#define __SERVO_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 舵机安全中位角。上电归中、轨迹初始化和异常恢复都以 90 度为基准。 */
#define SERVO_CENTER_ANGLE_DEG (90.0f)

/* 启动 TIM2 的 4 路 PWM 并将所有舵机输出到中位。 */
void Servo_Init(void);

/* 设置指定舵机目标角。
 * servo_id: 0~3，对应前左、前右、后左、后右；
 * angle_deg: 角度单位为度，驱动内部会夹紧到机械安全范围。
 */
void Servo_SetAngle(uint8_t servo_id, float angle_deg);

/* 读取最近一次写入的夹紧后角度，用于状态上报和调试。 */
float Servo_GetAngle(uint8_t servo_id);

#ifdef __cplusplus
}
#endif

#endif /* __SERVO_DRIVER_H__ */
