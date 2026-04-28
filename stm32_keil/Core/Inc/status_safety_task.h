#ifndef __STATUS_SAFETY_TASK_H__
#define __STATUS_SAFETY_TASK_H__

#include <stdint.h>

#include "uart_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Creates Task_Status_TX (50ms, reports servo state) and Task_Safety (100ms, feeds IWDG) */
void StatusSafetyTask_Create(void);
void StatusSafety_SystemStateInit(void);
uint8_t StatusSafety_GetSystemState(void);
uint8_t StatusSafety_HandleInitHandshake(const UartHandshakePayload *payload);
void StatusSafety_RequestSystemStateTx(void);
uint32_t StatusSafety_GetStatusTxStackHighWaterMark(void);
uint32_t StatusSafety_GetSafetyStackHighWaterMark(void);

#ifdef __cplusplus
}
#endif

#endif /* __STATUS_SAFETY_TASK_H__ */
