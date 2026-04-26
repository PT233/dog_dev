#include "status_safety_task.h"

#include <string.h>

#include "cmsis_os.h"
#include "usart.h"
#include "iwdg.h"
#include "uart_protocol.h"
#include "traj_planner.h"

#define STATUS_TX_PERIOD_MS  50U
#define SAFETY_PERIOD_MS     100U

/* Frame v1 = header(2) + cmd(1) + len(1) + payload(N) + crc(2) + tail(1) */
#define STATUS_PAYLOAD_LEN_V1  ((uint8_t)(TRAJ_SERVO_COUNT * sizeof(ServoStateItem)))
#define STATUS_FRAME_LEN_V1    ((uint8_t)(7U + STATUS_PAYLOAD_LEN_V1))

/* Frame v2 = header(2) + cmd(1) + len(1) + payload_v2(N) + crc(2) + tail(1) */
#define STATUS_PAYLOAD_LEN_V2  ((uint8_t)(TRAJ_SERVO_COUNT * sizeof(ServoStateItem_v2)))
#define STATUS_FRAME_LEN_V2    ((uint8_t)(7U + STATUS_PAYLOAD_LEN_V2))

#define STATUS_FRAME_LEN STATUS_FRAME_LEN_V2

static volatile uint16_t s_frame_seq = 0U;

static uint32_t StatusTX_GetTimestampMs(void)
{
    return (HAL_GetTick() % 65536U);
}

/* Pack and transmit a CMD_ID=0x82 status frame (v2) with timestamp and sequence */
static void StatusTX_SendFrame(void)
{
    uint8_t tx_buf[STATUS_FRAME_LEN_V2];
    uint16_t crc;
    uint8_t i;
    uint32_t timestamp_ms;
    uint16_t frame_seq;

    tx_buf[0] = UART_FRAME_HEADER_0;
    tx_buf[1] = UART_FRAME_HEADER_1;
    tx_buf[2] = (uint8_t)UART_CMD_SERVO_STATE_V2;
    tx_buf[3] = STATUS_PAYLOAD_LEN_V2;

    timestamp_ms = StatusTX_GetTimestampMs();
    frame_seq = s_frame_seq++;

    for (i = 0U; i < TRAJ_SERVO_COUNT; i++) {
        ServoStateItem_v2 item;
        item.servo_id           = i;
        item.current_angle_x10  = (int16_t)(g_traj_state[i].current_angle * 10.0f);
        item.status             = (g_traj_state[i].duration_ms > 0U) ? 1U : 0U;
        item.timestamp_ms       = (uint16_t)timestamp_ms;
        item.frame_seq          = frame_seq;
        memcpy(&tx_buf[4U + (uint8_t)(i * sizeof(ServoStateItem_v2))], &item, sizeof(ServoStateItem_v2));
    }

    crc = crc16_ccitt(&tx_buf[2], (size_t)(2U + STATUS_PAYLOAD_LEN_V2));
    tx_buf[4U + STATUS_PAYLOAD_LEN_V2] = (uint8_t)(crc & 0xFFU);
    tx_buf[5U + STATUS_PAYLOAD_LEN_V2] = (uint8_t)(crc >> 8);
    tx_buf[6U + STATUS_PAYLOAD_LEN_V2] = UART_FRAME_TAIL;

    /* Blocking transmit: ~31 bytes @ 921600 bps ≈ 0.27ms */
    HAL_UART_Transmit(&huart1, tx_buf, STATUS_FRAME_LEN_V2, 10U);
}

/* 20Hz status reporter: packs 4-servo state into 0x81 frame and sends via UART */
static void Task_Status_TX(void *arg)
{
    (void)arg;
    for (;;) {
        osDelay(STATUS_TX_PERIOD_MS);
        StatusTX_SendFrame();
    }
}

/* Safety watchdog feeder: feeds IWDG every 100ms.
 * Servo position is held automatically when no new Traj_SetTarget is called. */
static void Task_Safety(void *arg)
{
    (void)arg;
    for (;;) {
        osDelay(SAFETY_PERIOD_MS);
        HAL_IWDG_Refresh(&hiwdg);
    }
}

void StatusSafetyTask_Create(void)
{
    static const osThreadAttr_t tx_attr = {
        .name       = "StatusTX",
        .stack_size = 256U * 4U,
        .priority   = (osPriority_t)osPriorityNormal,
    };
    static const osThreadAttr_t safety_attr = {
        .name       = "Safety",
        .stack_size = 128U * 4U,
        .priority   = (osPriority_t)osPriorityAboveNormal,
    };
    osThreadNew(Task_Status_TX, NULL, &tx_attr);
    osThreadNew(Task_Safety,    NULL, &safety_attr);
}
