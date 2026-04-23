#ifndef __UART_RX_TASK_H__
#define __UART_RX_TASK_H__

#include "FreeRTOS.h"
#include "queue.h"
#include "uart_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

extern QueueHandle_t uart_rx_queue;
extern volatile uint32_t uart_crc_error_count;
extern volatile uint32_t uart_queue_drop_count;
extern volatile uint32_t uart_last_cmd_tick; /* HAL_GetTick() of last dispatched servo cmd */

void UartRxTask_Create(void);
void UartRxTask_NotifyFromIdleIrq(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_RX_TASK_H__ */
