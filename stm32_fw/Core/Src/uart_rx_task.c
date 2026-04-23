#include "uart_rx_task.h"

#include <string.h>

#include "task.h"
#include "usart.h"

#define UART_RX_DMA_BUF_LEN UART_MAX_FRAME_LEN
#define UART_RX_QUEUE_LEN 8U

QueueHandle_t uart_rx_queue = NULL;
volatile uint32_t uart_crc_error_count = 0;
volatile uint32_t uart_queue_drop_count = 0;

static TaskHandle_t s_uart_rx_task_handle = NULL;

static uint8_t s_uart_dma_buf[UART_RX_DMA_BUF_LEN];
static volatile uint16_t s_uart_dma_write_pos = 0;
static uint16_t s_uart_dma_read_pos = 0;

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

  ServoCmdItem item;

  (void)frame_len;

  payload_len = frame[3];
  crc_recv = (uint16_t)frame[4U + payload_len] | ((uint16_t)frame[5U + payload_len] << 8);
  crc_calc = crc16_ccitt(&frame[2], (size_t)(2U + payload_len));

  if (crc_calc != crc_recv)
  {
    uart_crc_error_count++;
    return;
  }

  if ((frame[2] != UART_CMD_SERVO_CONTROL) || (payload_len < (uint8_t)sizeof(ServoCmdItem)))
  {
    return;
  }

  memcpy(&item, &frame[4], sizeof(ServoCmdItem));

  if (uart_rx_queue == NULL)
  {
    return;
  }

  if (xQueueSend(uart_rx_queue, &item, 0) != pdPASS)
  {
    uart_queue_drop_count++;
  }
}

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

  __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);

  for (;;)
  {
    (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    UartRx_ProcessDmaData();
  }
}

void UartRxTask_Create(void)
{
  if (uart_rx_queue == NULL)
  {
    uart_rx_queue = xQueueCreate(UART_RX_QUEUE_LEN, sizeof(ServoCmdItem));
    configASSERT(uart_rx_queue != NULL);
  }

  if (s_uart_rx_task_handle == NULL)
  {
    (void)xTaskCreate(Task_UART_RX,
                      "Task_UART_RX",
                      256,
                      NULL,
                      (tskIDLE_PRIORITY + 2U),
                      &s_uart_rx_task_handle);
  }
}

void UartRxTask_NotifyFromIdleIrq(void)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (huart1.hdmarx == NULL)
  {
    return;
  }

  s_uart_dma_write_pos = (uint16_t)(UART_RX_DMA_BUF_LEN - __HAL_DMA_GET_COUNTER(huart1.hdmarx));

  if (s_uart_rx_task_handle != NULL)
  {
    vTaskNotifyGiveFromISR(s_uart_rx_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}
