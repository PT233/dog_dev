#ifndef __STATUS_SAFETY_TASK_H__
#define __STATUS_SAFETY_TASK_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Creates Task_Status_TX (50ms, reports servo state) and Task_Safety (100ms, feeds IWDG) */
void StatusSafetyTask_Create(void);

#ifdef __cplusplus
}
#endif

#endif /* __STATUS_SAFETY_TASK_H__ */
