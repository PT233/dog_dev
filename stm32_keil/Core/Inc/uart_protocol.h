#ifndef STM32_KEIL__CORE__INC__UART_PROTOCOL_H_
#define STM32_KEIL__CORE__INC__UART_PROTOCOL_H_

/* UART 二进制协议定义
 *
 * 该头文件是 STM32 与上位 ROS 2 uart_bridge 的线协议契约：
 *   [0xAA][0x55][CMD_ID][LEN][PAYLOAD...][CRC16_LO][CRC16_HI][0x0D]
 *
 * 设计要点：
 * - 所有结构体使用 packed，保证 C/C++ 两端按字节一致解析；
 * - 多字节整数按小端 MCU 的内存顺序直接传输；
 * - CRC16 覆盖 CMD_ID、LEN 和 PAYLOAD，不覆盖帧头与帧尾；
 * - PROTOCOL_VERSION 改动代表 payload 语义不兼容，需要两端同步升级。
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  /* STM32_KEIL__CORE__INC__UART_PROTOCOL_H_ */

#define UART_FRAME_HEADER_0 ((uint8_t)0xAA)  /* 帧头第 1 字节，用于在字节流中快速找同步点 */
#define UART_FRAME_HEADER_1 ((uint8_t)0x55)  /* 帧头第 2 字节，降低误判普通 payload 为帧头的概率 */
#define UART_FRAME_TAIL ((uint8_t)0x0D)      /* 帧尾字节，作为长度字段之外的最后一道完整性检查 */

#define UART_MAX_FRAME_LEN 256               /* 含帧头、命令、长度、payload、CRC 和帧尾的最大总长度 */

#define UART_PROTOCOL_VERSION ((uint8_t)0x03) /* 握手版本号，ROS 2 和 STM32 不一致时拒绝进入 ACTIVE */

typedef enum {
    kUartCmdServoControl = 0x01,   /* ROS 2 -> STM32：一帧内可携带多路舵机目标角 */
    kUartCmdQuery = 0x02,           /* 预留查询命令，当前主流程未使用 */
    kUartCmdInitHandshake = 0x10,  /* ROS 2 -> STM32：请求完成启动握手并进入可控状态 */
    kUartCmdServoState = 0x81,     /* STM32 -> ROS 2：舵机状态 v1，无时间戳，保留兼容 */
    kUartCmdServoStateV2 = 0x82,  /* STM32 -> ROS 2：舵机状态 v2，含时间戳和序号 */
    kUartCmdSystemState = 0x83,    /* STM32 -> ROS 2：启动/等待/激活状态上报 */
    kUartCmdEmergencyStop = 0xFF   /* 预留紧急停止命令，双向语义 */
} UartCmdId;

typedef enum {
    kUartSystemStateBootCentering = 0x01,      /* 上电归中阶段，舵机先回到安全中位 */
    kUartSystemStateWaitingConnection = 0x02,  /* 归中完成，等待 ROS 2 握手 */
    kUartSystemStateActive = 0x03,              /* 握手完成，允许执行舵机控制帧 */
    kUartSystemStateError = 0x7F                /* 预留错误状态，当前故障主要由 IWDG 复位兜底 */
} UartSystemState;

typedef struct __attribute__((packed)) {
    uint8_t servo_id;      /* 0=front_left, 1=front_right, 2=rear_left, 3=rear_right */
    int16_t angle_x10;     /* 目标角度放大 10 倍传输，避免浮点跨端序列化差异 */
    uint16_t duration_ms;  /* 期望运动时长，轨迹规划器据此生成平滑过渡 */
} ServoCmdItem;

typedef struct __attribute__((packed)) {
    uint8_t servo_id;            /* 舵机编号，与 ROS JointState name 的映射保持一致 */
    int16_t current_angle_x10;    /* 当前输出角度 ×10 */
    uint8_t status;              /* 0=idle，1=moving */
} ServoStateItem;

typedef struct __attribute__((packed)) {
    uint8_t servo_id;            /* 舵机编号 */
    int16_t current_angle_x10;    /* 当前输出角度 ×10 */
    uint8_t status;              /* 0=idle，1=moving */
    uint16_t timestamp_ms;       /* STM32 端 16-bit 毫秒时间戳，用于上位机估算链路延迟 */
    uint16_t frame_seq;          /* 状态帧序号，用于 uart_bridge 统计丢帧 */
} ServoStateItemV2;

typedef struct __attribute__((packed)) {
    uint8_t protocol_version;    /* 上位机声明的协议版本 */
    uint8_t requested_state;     /* 期望 STM32 进入的状态，正常为 kUartSystemStateActive */
    uint16_t reserved;           /* 保留字段，保持 4 字节对齐并给后续扩展留空间 */
} UartHandshakePayload;

typedef struct __attribute__((packed)) {
    uint8_t protocol_version;    /* STM32 当前协议版本 */
    uint8_t system_state;        /* UartSystemState 枚举值 */
    uint16_t reserved;           /* 保留字段 */
    uint32_t uptime_ms;          /* HAL_GetTick() 上电运行时间，便于日志定位启动阶段 */
} UartSystemStatePayload;

/* CRC16-CCITT，初始值 0xFFFF，多项式 0x1021。 */
uint16_t crc16_ccitt(const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
