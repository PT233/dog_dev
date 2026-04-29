// STM32 SG90 舵机 PWM 驱动（TIM2，4 路）
// PWM 频率：50Hz（周期 20ms），定时器时钟 1MHz（ARR=19999）
// 脉宽与角度关系：PWM = 500 + angle × (2000/180) μs
//   0°  → 500μs（CCR=500）
//   90° → 1500μs（CCR=1500）
//   180°→ 2500μs（CCR=2500）
#include "servo_driver.h"

#include "tim.h"

#define SERVO_COUNT (4U)
#define SERVO_MIN_ANGLE_DEG (0.0f)
#define SERVO_MAX_ANGLE_DEG (180.0f)
#define SERVO_CCR_BASE (500.0f)            // 对应 0°（0.5ms 脉宽）
#define SERVO_CCR_PER_DEG (2000.0f / 180.0f)  // 每度对应 CCR 增量 ≈ 11.11

/* TIM2 的 4 个通道分别接 4 路腿舵机。
 * 通道顺序必须与协议中的 servo_id 顺序一致，否则 ROS 2 下发的关节名会驱错腿。
 */
static const uint32_t k_servo_channels[SERVO_COUNT] = {
  TIM_CHANNEL_1,
  TIM_CHANNEL_2,
  TIM_CHANNEL_3,
  TIM_CHANNEL_4
};

static float g_last_angle_deg[SERVO_COUNT] = {
  SERVO_CENTER_ANGLE_DEG,
  SERVO_CENTER_ANGLE_DEG,
  SERVO_CENTER_ANGLE_DEG,
  SERVO_CENTER_ANGLE_DEG
};

/* 将外部输入夹紧到舵机可接受范围，避免错误指令撞机械限位。 */
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
  /* PWM 启动后立即写中位角，避免上电阶段保留未知 CCR 导致舵机突然摆动。 */
  for (uint8_t i = 0; i < SERVO_COUNT; ++i)
  {
    if (HAL_TIM_PWM_Start(&htim2, k_servo_channels[i]) != HAL_OK)
    {
      Error_Handler();
    }
    Servo_SetAngle(i, SERVO_CENTER_ANGLE_DEG);
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
  /* 定时器计数频率为 1 MHz，因此 CCR 数值等价于高电平微秒数。 */
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
