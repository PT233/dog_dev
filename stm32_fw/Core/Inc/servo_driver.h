#ifndef __SERVO_DRIVER_H__
#define __SERVO_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Servo_Init(void);
void Servo_SetAngle(uint8_t servo_id, float angle_deg);
float Servo_GetAngle(uint8_t servo_id);

#ifdef __cplusplus
}
#endif

#endif /* __SERVO_DRIVER_H__ */
