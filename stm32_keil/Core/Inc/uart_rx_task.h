#ifndef __UART_RX_TASK_H__
#define __UART_RX_TASK_H__

#include "FreeRTOS.h"
#include "queue.h"
#include "uart_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 解析出的舵机控制项队列，由 UART 接收任务写入并立即分发到轨迹规划器。 */
extern QueueHandle_t uart_rx_queue;

/* 运行时诊断计数：CRC 错帧数、队列满丢弃数、最后一次有效指令时间。 */
extern volatile uint32_t uart_crc_error_count;
extern volatile uint32_t uart_queue_drop_count;
extern volatile uint32_t uart_last_cmd_tick; /* HAL_GetTick() of last dispatched servo cmd */

/* 创建 UART RX 任务和内部队列。 */
void UartRxTask_Create(void);

/* USART IDLE 中断入口调用：记录 DMA 写入位置并唤醒解析任务。 */
void UartRxTask_NotifyFromIdleIrq(void);

/* 返回 UART RX 任务剩余栈水位，用于判断是否存在栈空间风险。 */
uint32_t UartRxTask_GetStackHighWaterMark(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_RX_TASK_H__ */
