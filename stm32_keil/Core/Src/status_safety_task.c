#include "status_safety_task.h"

#include <string.h>

#include "cmsis_os.h"
#include "task.h"
#include "usart.h"
#include "iwdg.h"
#include "uart_protocol.h"
#include "traj_planner.h"

#define STATUS_TX_PERIOD_MS  50U      /* 20Hz 状态回传频率，兼顾实时性和串口带宽 */
#define SAFETY_PERIOD_MS     100U     /* IWDG 喂狗周期，必须短于 iwdg.c 中配置的超时时间 */
#define BOOT_CENTER_HOLD_MS  500U     /* 上电后舵机保持中位的最短时间，防止机械突然动作 */
#define SYSTEM_STATE_TX_FLAG (1U << 0) /* 事件标志：请求立即发送系统状态帧 */

/* Frame v1 = header(2) + cmd(1) + len(1) + payload(N) + crc(2) + tail(1) */
#define STATUS_PAYLOAD_LEN_V1  ((uint8_t)(TRAJ_SERVO_COUNT * sizeof(ServoStateItem)))
#define STATUS_FRAME_LEN_V1    ((uint8_t)(7U + STATUS_PAYLOAD_LEN_V1))

/* Frame v2 = header(2) + cmd(1) + len(1) + payload_v2(N) + crc(2) + tail(1) */
#define STATUS_PAYLOAD_LEN_V2  ((uint8_t)(TRAJ_SERVO_COUNT * sizeof(ServoStateItem_v2)))
#define STATUS_FRAME_LEN_V2    ((uint8_t)(7U + STATUS_PAYLOAD_LEN_V2))

#define STATUS_FRAME_LEN STATUS_FRAME_LEN_V2

/* System state frame = header(2) + cmd(1) + len(1) + payload + crc(2) + tail(1) */
#define SYSTEM_STATE_PAYLOAD_LEN  ((uint8_t)sizeof(UartSystemStatePayload))
#define SYSTEM_STATE_FRAME_LEN    ((uint8_t)(7U + SYSTEM_STATE_PAYLOAD_LEN))

static volatile uint16_t s_frame_seq = 0U;
static volatile uint8_t s_system_state = UART_SYSTEM_STATE_BOOT_CENTERING;
static uint32_t s_center_start_tick = 0U;
static osThreadId_t s_status_tx_handle = NULL;
static volatile uint32_t s_status_tx_stack_high_water_mark = 0U;
static volatile uint32_t s_safety_stack_high_water_mark = 0U;

void StatusSafety_SystemStateInit(void)
{
    /* main.c 在启动调度器前调用，记录舵机归中开始时间。 */
    s_center_start_tick = HAL_GetTick();
    s_system_state = UART_SYSTEM_STATE_BOOT_CENTERING;
}

static void SystemState_Update(void)
{
    /* 归中保持时间到后才进入等待连接，期间 UART 控制帧会被拒绝。 */
    if ((s_system_state == UART_SYSTEM_STATE_BOOT_CENTERING) &&
        ((HAL_GetTick() - s_center_start_tick) >= BOOT_CENTER_HOLD_MS)) {
        s_system_state = UART_SYSTEM_STATE_WAITING_CONNECTION;
    }
}

uint8_t StatusSafety_GetSystemState(void)
{
    SystemState_Update();
    return s_system_state;
}

uint8_t StatusSafety_HandleInitHandshake(const UartHandshakePayload *payload)
{
    SystemState_Update();

    /* 只接受协议版本一致且请求 ACTIVE 的握手。
     * 若 STM32 还处于 BOOT_CENTERING，则不会提前激活，只回报当前状态。
     */
    if ((payload != NULL) &&
        (payload->protocol_version == UART_PROTOCOL_VERSION) &&
        (payload->requested_state == UART_SYSTEM_STATE_ACTIVE)) {
        if ((s_system_state == UART_SYSTEM_STATE_WAITING_CONNECTION) ||
            (s_system_state == UART_SYSTEM_STATE_ACTIVE)) {
            s_system_state = UART_SYSTEM_STATE_ACTIVE;
        }
    }

    StatusSafety_RequestSystemStateTx();
    return s_system_state;
}

void StatusSafety_RequestSystemStateTx(void)
{
    if (s_status_tx_handle != NULL) {
        (void)osThreadFlagsSet(s_status_tx_handle, SYSTEM_STATE_TX_FLAG);
    }
}

static uint32_t StatusTX_GetTimestampMs(void)
{
    /* 状态 payload 里只有 16-bit timestamp_ms，先在这里取模保持与字段宽度一致。 */
    return (HAL_GetTick() % 65536U);
}

/* 打包并发送 CMD_ID=0x82 舵机状态帧。
 * 每路舵机携带同一个 timestamp 和 frame_seq，ROS 端据此估算延迟与丢帧。
 */
static void StatusTX_SendFrame(void)
{
    uint8_t tx_buf[STATUS_FRAME_LEN_V2];
    TrajState traj_snapshot[TRAJ_SERVO_COUNT];
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
    /* 复制轨迹快照，避免打包过程中轨迹任务改写一半字段。 */
    TrajPlanner_CopyStateSnapshot(traj_snapshot);

    for (i = 0U; i < TRAJ_SERVO_COUNT; i++) {
        ServoStateItem_v2 item;
        item.servo_id           = i;
        item.current_angle_x10  = (int16_t)(traj_snapshot[i].current_angle * 10.0f);
        item.status             = (traj_snapshot[i].duration_ms > 0U) ? 1U : 0U;
        item.timestamp_ms       = (uint16_t)timestamp_ms;
        item.frame_seq          = frame_seq;
        memcpy(&tx_buf[4U + (uint8_t)(i * sizeof(ServoStateItem_v2))], &item, sizeof(ServoStateItem_v2));
    }

    crc = crc16_ccitt(&tx_buf[2], (size_t)(2U + STATUS_PAYLOAD_LEN_V2));
    tx_buf[4U + STATUS_PAYLOAD_LEN_V2] = (uint8_t)(crc & 0xFFU);
    tx_buf[5U + STATUS_PAYLOAD_LEN_V2] = (uint8_t)(crc >> 8);
    tx_buf[6U + STATUS_PAYLOAD_LEN_V2] = UART_FRAME_TAIL;

    /* 阻塞发送：约 31 字节 @ 921600 bps ≈ 0.27ms，远小于 50ms 周期。 */
    if (HAL_UART_Transmit(&huart1, tx_buf, STATUS_FRAME_LEN_V2, 10U) != HAL_OK) {
        Error_Handler();
    }
}

static void StatusTX_SendSystemStateFrame(void)
{
    uint8_t tx_buf[SYSTEM_STATE_FRAME_LEN];
    UartSystemStatePayload payload;
    uint16_t crc;

    payload.protocol_version = UART_PROTOCOL_VERSION;
    /* 系统状态帧用于握手闭环：上位机只有看到 ACTIVE 才开始下发控制。 */
    payload.system_state = StatusSafety_GetSystemState();
    payload.reserved = 0U;
    payload.uptime_ms = HAL_GetTick();

    tx_buf[0] = UART_FRAME_HEADER_0;
    tx_buf[1] = UART_FRAME_HEADER_1;
    tx_buf[2] = (uint8_t)UART_CMD_SYSTEM_STATE;
    tx_buf[3] = SYSTEM_STATE_PAYLOAD_LEN;
    memcpy(&tx_buf[4], &payload, sizeof(payload));

    crc = crc16_ccitt(&tx_buf[2], (size_t)(2U + SYSTEM_STATE_PAYLOAD_LEN));
    tx_buf[4U + SYSTEM_STATE_PAYLOAD_LEN] = (uint8_t)(crc & 0xFFU);
    tx_buf[5U + SYSTEM_STATE_PAYLOAD_LEN] = (uint8_t)(crc >> 8);
    tx_buf[6U + SYSTEM_STATE_PAYLOAD_LEN] = UART_FRAME_TAIL;

    if (HAL_UART_Transmit(&huart1, tx_buf, SYSTEM_STATE_FRAME_LEN, 10U) != HAL_OK) {
        Error_Handler();
    }
}

/* 20Hz 状态上报任务：周期发送舵机状态；收到事件标志时插入一帧系统状态。 */
static void Task_Status_TX(void *arg)
{
    (void)arg;
    s_status_tx_stack_high_water_mark = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    for (;;) {
        uint32_t flags = osThreadFlagsWait(SYSTEM_STATE_TX_FLAG,
                                           osFlagsWaitAny,
                                           STATUS_TX_PERIOD_MS);
        s_status_tx_stack_high_water_mark = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
        SystemState_Update();
        if (((flags & osFlagsError) == 0U) && ((flags & SYSTEM_STATE_TX_FLAG) != 0U)) {
            StatusTX_SendSystemStateFrame();
        }
        StatusTX_SendFrame();
    }
}

/* 安全任务：100ms 喂一次 IWDG。
 * 若调度器、该任务或关键中断卡死，独立看门狗会复位 MCU。
 * 无新 Traj_SetTarget 时舵机会保持最后输出角，不在这里主动回中。
 */
static void Task_Safety(void *arg)
{
    (void)arg;
    s_safety_stack_high_water_mark = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    for (;;) {
        osDelay(SAFETY_PERIOD_MS);
        s_safety_stack_high_water_mark = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
        if (HAL_IWDG_Refresh(&hiwdg) != HAL_OK) {
            Error_Handler();
        }
    }
}

uint32_t StatusSafety_GetStatusTxStackHighWaterMark(void)
{
    return s_status_tx_stack_high_water_mark;
}

uint32_t StatusSafety_GetSafetyStackHighWaterMark(void)
{
    return s_safety_stack_high_water_mark;
}

void StatusSafetyTask_Create(void)
{
    osThreadId_t safety_handle;
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
    /* 保存状态任务句柄，用于其他模块通过 osThreadFlagsSet 触发立即上报。 */
    s_status_tx_handle = osThreadNew(Task_Status_TX, NULL, &tx_attr);
    configASSERT(s_status_tx_handle != NULL);

    safety_handle = osThreadNew(Task_Safety, NULL, &safety_attr);
    configASSERT(safety_handle != NULL);
}
