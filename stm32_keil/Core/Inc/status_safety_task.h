#ifndef __STATUS_SAFETY_TASK_H__
#define __STATUS_SAFETY_TASK_H__

#include <stdint.h>

#include "uart_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 创建状态上报任务和安全看门狗任务：
 * - StatusTX: 周期发送舵机状态帧，并按需发送系统状态帧；
 * - Safety: 周期喂独立看门狗，任务卡死时触发 MCU 复位。
 */
void StatusSafetyTask_Create(void);

/* 初始化系统状态机，上电后先进入 BOOT_CENTERING。 */
void StatusSafety_SystemStateInit(void);

/* 获取当前系统状态；内部会自动推进归中超时状态。 */
uint8_t StatusSafety_GetSystemState(void);

/* 处理上位机握手帧，版本匹配且状态允许时进入 ACTIVE。 */
uint8_t StatusSafety_HandleInitHandshake(const UartHandshakePayload *payload);

/* 请求状态上报任务尽快发一帧 SYSTEM_STATE，通常用于拒绝指令时提示上位机原因。 */
void StatusSafety_RequestSystemStateTx(void);

/* 栈水位诊断接口。 */
uint32_t StatusSafety_GetStatusTxStackHighWaterMark(void);
uint32_t StatusSafety_GetSafetyStackHighWaterMark(void);

#ifdef __cplusplus
}
#endif

#endif /* __STATUS_SAFETY_TASK_H__ */
