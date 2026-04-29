#include "uart_rx_task.h"

#include <string.h>

#include "task.h"
#include "usart.h"
#include "traj_planner.h"
#include "status_safety_task.h"

#define UART_RX_DMA_BUF_LEN UART_MAX_FRAME_LEN
#define UART_RX_QUEUE_LEN 8U

/* 对外暴露的诊断状态，方便后续通过状态帧或调试器观察链路质量。 */
QueueHandle_t uart_rx_queue = NULL;
volatile uint32_t uart_crc_error_count  = 0;
volatile uint32_t uart_queue_drop_count = 0;
volatile uint32_t uart_last_cmd_tick    = 0;

static TaskHandle_t s_uart_rx_task_handle = NULL;
static volatile uint32_t s_uart_rx_stack_high_water_mark = 0U;

/* DMA 环形接收缓冲。USART1 IDLE 中断只更新写位置，解析工作放到任务上下文完成。 */
static uint8_t s_uart_dma_buf[UART_RX_DMA_BUF_LEN];
static volatile uint16_t s_uart_dma_write_pos = 0;
static uint16_t s_uart_dma_read_pos = 0;

/* 当前正在拼接的一帧数据。状态机按字节推进，不依赖一次 DMA 收满整帧。 */
static uint8_t s_frame_buf[UART_MAX_FRAME_LEN];
static uint16_t s_frame_len = 0;

static void UartRx_ResetFrameParser(void)
{
  s_frame_len = 0;
}

static void UartRx_HandleFrame(const uint8_t* frame, uint16_t frame_len)
{
  uint8_t payload_len;
  uint16_t crc_calc;
  uint16_t crc_recv;
  uint8_t cmd_id;

  (void)frame_len;  /* 长度已经由解析器按 LEN 字段校验，这里只保留接口一致性。 */

  cmd_id = frame[2];
  payload_len = frame[3];
  crc_recv = (uint16_t)frame[4U + payload_len] | ((uint16_t)frame[5U + payload_len] << 8);
  crc_calc = crc16_ccitt(&frame[2], (size_t)(2U + payload_len));

  if (crc_calc != crc_recv)
  {
    /* CRC 错误通常来自波特率不匹配、线缆干扰或上位机帧格式不一致。 */
    uart_crc_error_count++;
    return;
  }

  if (cmd_id == kUartCmdInitHandshake) {
    UartHandshakePayload handshake;

    /* payload 太短时仍触发系统状态回包，让上位机能看到当前 STM32 状态。 */
    if (payload_len < (uint8_t)sizeof(UartHandshakePayload)) {
      (void)StatusSafety_HandleInitHandshake(NULL);
      return;
    }

    memcpy(&handshake, &frame[4], sizeof(UartHandshakePayload));
    (void)StatusSafety_HandleInitHandshake(&handshake);
    return;
  }

  if (cmd_id != kUartCmdServoControl)
  {
    /* 未实现的命令先静默丢弃，避免未知 payload 影响实时控制链路。 */
    return;
  }

  if (StatusSafety_GetSystemState() != kUartSystemStateActive)
  {
    /* 只有握手完成后才允许执行舵机指令，防止上电归中阶段被上位机抢控制权。 */
    StatusSafety_RequestSystemStateTx();
    return;
  }

  if ((payload_len < (uint8_t)sizeof(ServoCmdItem)) ||
      ((payload_len % (uint8_t)sizeof(ServoCmdItem)) != 0U))
  {
    return;
  }

  if (uart_rx_queue == NULL)
  {
    return;
  }

  for (uint8_t offset = 0U; offset < payload_len; offset = (uint8_t)(offset + sizeof(ServoCmdItem)))
  {
    ServoCmdItem item;
    memcpy(&item, &frame[4U + offset], sizeof(ServoCmdItem));

    if (xQueueSend(uart_rx_queue, &item, 0) != pdPASS)
    {
      /* 不在这里阻塞等待队列空间，避免接收任务被慢控制链拖住导致后续帧堆积。 */
      uart_queue_drop_count++;
    }
  }
}

/* UART 帧字节级状态机：
 * 找到 0xAA 0x55 后读取 CMD、LEN、payload、CRC 和尾字节。
 * 任意非法长度或尾字节错误都会重置状态，保证能从下一帧重新同步。
 */
static void UartRx_ParseByte(uint8_t byte)
{
  uint16_t expected_len;

  if (s_frame_len == 0U)
  {
    if (byte == UART_FRAME_HEADER_0)
    {
      s_frame_buf[s_frame_len++] = byte;
    }
    return;
  }

  if (s_frame_len == 1U)
  {
    if (byte == UART_FRAME_HEADER_1)
    {
      s_frame_buf[s_frame_len++] = byte;
      return;
    }

    if (byte == UART_FRAME_HEADER_0)
    {
      s_frame_buf[0] = byte;
      return;
    }

    UartRx_ResetFrameParser();
    return;
  }

  if (s_frame_len >= UART_MAX_FRAME_LEN)
  {
    UartRx_ResetFrameParser();
    return;
  }

  s_frame_buf[s_frame_len++] = byte;

  if (s_frame_len < 4U)
  {
    return;
  }

  expected_len = (uint16_t)(7U + s_frame_buf[3]);

  if ((expected_len < 7U) || (expected_len > UART_MAX_FRAME_LEN))
  {
    UartRx_ResetFrameParser();
    return;
  }

  if (s_frame_len == expected_len)
  {
    if (s_frame_buf[expected_len - 1U] == UART_FRAME_TAIL)
    {
      UartRx_HandleFrame(s_frame_buf, expected_len);
    }

    UartRx_ResetFrameParser();
  }
}

static void UartRx_ProcessDmaData(void)
{
  uint16_t write_pos;

  write_pos = s_uart_dma_write_pos;

  /* 处理 DMA 环形缓冲中从 read_pos 到 write_pos 的增量数据。 */
  while (s_uart_dma_read_pos != write_pos)
  {
    UartRx_ParseByte(s_uart_dma_buf[s_uart_dma_read_pos]);

    s_uart_dma_read_pos++;
    if (s_uart_dma_read_pos >= UART_RX_DMA_BUF_LEN)
    {
      s_uart_dma_read_pos = 0U;
    }
  }
}

static void Task_UART_RX(void* argument)
{
  (void)argument;

  if (HAL_UART_Receive_DMA(&huart1, s_uart_dma_buf, UART_RX_DMA_BUF_LEN) != HAL_OK)
  {
    Error_Handler();
  }

  /* 仅使用 IDLE 中断作为一批数据到达的信号，关闭半满中断减少 ISR 抖动。 */
  __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
  __HAL_UART_CLEAR_IDLEFLAG(&huart1);
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
  s_uart_rx_stack_high_water_mark = (uint32_t)uxTaskGetStackHighWaterMark(NULL);

  for (;;)
  {
    ServoCmdItem item;

    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    s_uart_rx_stack_high_water_mark = (uint32_t)uxTaskGetStackHighWaterMark(NULL);
    UartRx_ProcessDmaData();

    /* 将解析出的舵机命令全部分发给轨迹规划器。 */
    while (xQueueReceive(uart_rx_queue, &item, 0) == pdPASS)
    {
      Traj_SetTarget(item.servo_id, (float)item.angle_x10 / 10.0f, item.duration_ms);
      uart_last_cmd_tick = HAL_GetTick();
    }
  }
}

void UartRxTask_Create(void)
{
  BaseType_t status;

  if (uart_rx_queue == NULL)
  {
    /* 队列只承载解析后的 ServoCmdItem，长度 8 足够容纳两帧四舵机批量指令。 */
    uart_rx_queue = xQueueCreate(UART_RX_QUEUE_LEN, sizeof(ServoCmdItem));
    configASSERT(uart_rx_queue != NULL);
  }

  if (s_uart_rx_task_handle == NULL)
  {
    status = xTaskCreate(Task_UART_RX,
                         "Task_UART_RX",
                         256,
                         NULL,
                         (tskIDLE_PRIORITY + 2U),
                         &s_uart_rx_task_handle);
    configASSERT(status == pdPASS);
  }
}

uint32_t UartRxTask_GetStackHighWaterMark(void)
{
  return s_uart_rx_stack_high_water_mark;
}

void UartRxTask_NotifyFromIdleIrq(void)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (huart1.hdmarx == NULL)
  {
    return;
  }

  /* DMA 剩余计数转换为当前写指针；任务会从上次 read_pos 追到这个位置。 */
  s_uart_dma_write_pos = (uint16_t)(UART_RX_DMA_BUF_LEN - __HAL_DMA_GET_COUNTER(huart1.hdmarx));

  if (s_uart_rx_task_handle != NULL)
  {
    vTaskNotifyGiveFromISR(s_uart_rx_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}
