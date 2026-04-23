#include "servo_driver.h"

#include "tim.h"

#define SERVO_COUNT (4U)
#define SERVO_MIN_ANGLE_DEG (0.0f)
#define SERVO_MAX_ANGLE_DEG (180.0f)
#define SERVO_CCR_BASE (500.0f)
#define SERVO_CCR_PER_DEG (2000.0f / 180.0f)

static const uint32_t k_servo_channels[SERVO_COUNT] = {
  TIM_CHANNEL_1,
  TIM_CHANNEL_2,
  TIM_CHANNEL_3,
  TIM_CHANNEL_4
};

static float g_last_angle_deg[SERVO_COUNT] = {0.0f, 0.0f, 0.0f, 0.0f};

static float Servo_ClampAngle(float angle_deg)
{
  if (angle_deg < SERVO_MIN_ANGLE_DEG)
  {
    return SERVO_MIN_ANGLE_DEG;
  }

  if (angle_deg > SERVO_MAX_ANGLE_DEG)
  {
    return SERVO_MAX_ANGLE_DEG;
  }

  return angle_deg;
}

void Servo_Init(void)
{
  for (uint8_t i = 0; i < SERVO_COUNT; ++i)
  {
    HAL_TIM_PWM_Start(&htim2, k_servo_channels[i]);
    Servo_SetAngle(i, 0.0f);
  }
}

void Servo_SetAngle(uint8_t servo_id, float angle_deg)
{
  float clamped_angle;
  uint32_t ccr;

  if (servo_id >= SERVO_COUNT)
  {
    return;
  }

  clamped_angle = Servo_ClampAngle(angle_deg);
  ccr = (uint32_t)(SERVO_CCR_BASE + (clamped_angle * SERVO_CCR_PER_DEG));

  __HAL_TIM_SET_COMPARE(&htim2, k_servo_channels[servo_id], ccr);
  g_last_angle_deg[servo_id] = clamped_angle;
}

float Servo_GetAngle(uint8_t servo_id)
{
  if (servo_id >= SERVO_COUNT)
  {
    return 0.0f;
  }

  return g_last_angle_deg[servo_id];
}
