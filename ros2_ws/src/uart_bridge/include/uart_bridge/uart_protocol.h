#ifndef SHARED_UART_PROTOCOL_H
#define SHARED_UART_PROTOCOL_H

// UART 二进制协议定义（树莓派端与 STM32 端共用此头文件）
//
// 帧格式：[0xAA][0x55][CMD_ID][LEN][PAYLOAD...][CRC16_LO][CRC16_HI][0x0D]
// CRC16：CCITT 算法，覆盖 CMD_ID + LEN + PAYLOAD，小端序发送

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UART_FRAME_HEADER_0 ((uint8_t)0xAA)  // 帧头第 1 字节
#define UART_FRAME_HEADER_1 ((uint8_t)0x55)  // 帧头第 2 字节
#define UART_FRAME_TAIL     ((uint8_t)0x0D)  // 帧尾

#define UART_MAX_FRAME_LEN 256  // 最大帧长度（含帧头/帧尾/CRC）

#define UART_PROTOCOL_VERSION ((uint8_t)0x03)  // 握手时版本号校验

// 命令 ID：区分帧的类型和方向
typedef enum {
    UART_CMD_SERVO_CONTROL   = 0x01,  // 树莓派 → STM32：设置舵机目标角度
    UART_CMD_QUERY           = 0x02,  // 树莓派 → STM32：查询状态
    UART_CMD_INIT_HANDSHAKE  = 0x10,  // 树莓派 → STM32：建立连接握手
    UART_CMD_SERVO_STATE     = 0x81,  // STM32 → 树莓派：舵机状态 v1（无时间戳）
    UART_CMD_SERVO_STATE_V2  = 0x82,  // STM32 → 树莓派：舵机状态 v2（含时间戳+序列号）
    UART_CMD_SYSTEM_STATE    = 0x83,  // STM32 → 树莓派：系统状态（启动阶段上报）
    UART_CMD_EMERGENCY_STOP  = 0xFF   // 双向：紧急停止
} UartCmdId;

// STM32 系统状态枚举（在 SYSTEM_STATE 帧中上报）
typedef enum {
    UART_SYSTEM_STATE_BOOT_CENTERING     = 0x01,  // 正在初始化归中
    UART_SYSTEM_STATE_WAITING_CONNECTION = 0x02,  // 等待握手
    UART_SYSTEM_STATE_ACTIVE             = 0x03,  // 就绪，可接受舵机指令
    UART_SYSTEM_STATE_ERROR              = 0x7F   // 故障（IWDG 超时后重启）
} UartSystemState;

// 单路腿舵机控制指令（一帧可包含多个，用于批量发送）
typedef struct __attribute__((packed)) {
    uint8_t  servo_id;       // 0=front_left, 1=front_right, 2=rear_left, 3=rear_right
    int16_t  angle_x10;      // 目标角度 × 10，如 900 代表 90.0°
    uint16_t duration_ms;    // 期望运动时长（ms），供梯形规划使用
} ServoCmdItem;              // sizeof = 5 字节

// 舵机状态反馈 v1（不含时间戳，用于简单状态查询）
typedef struct __attribute__((packed)) {
    uint8_t servo_id;
    int16_t current_angle_x10;  // 当前角度 × 10
    uint8_t status;             // 0=idle, 1=moving
} ServoStateItem;              // sizeof = 4 字节

// 舵机状态反馈 v2（含 STM32 时间戳和帧序列号，用于延迟测量和丢帧检测）
typedef struct __attribute__((packed)) {
    uint8_t  servo_id;
    int16_t  current_angle_x10;
    uint8_t  status;
    uint16_t timestamp_ms;   // STM32 HAL_GetTick() 时间戳（用于时间戳映射）
    uint16_t frame_seq;      // 状态帧序列号（同一帧各舵机相同，用于丢帧统计）
} ServoStateItem_v2;        // sizeof = 8 字节

// 握手 payload（CMD_ID = 0x10，树莓派 → STM32）
typedef struct __attribute__((packed)) {
    uint8_t  protocol_version;   // 必须与 STM32 的 UART_PROTOCOL_VERSION 一致
    uint8_t  requested_state;    // 请求 STM32 进入 ACTIVE 状态
    uint16_t reserved;           // 保留字段，填 0
} UartHandshakePayload;

// 系统状态 payload（CMD_ID = 0x83，STM32 → 树莓派）
typedef struct __attribute__((packed)) {
    uint8_t  protocol_version;
    uint8_t  system_state;   // UartSystemState 枚举值
    uint16_t reserved;
    uint32_t uptime_ms;      // STM32 上电运行时间（HAL_GetTick()）
} UartSystemStatePayload;

// CRC16-CCITT 计算（多项式 0x1021，初始值 0xFFFF）
uint16_t crc16_ccitt(const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
