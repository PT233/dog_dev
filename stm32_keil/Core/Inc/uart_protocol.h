#ifndef SHARED_UART_PROTOCOL_H
#define SHARED_UART_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UART_FRAME_HEADER_0 ((uint8_t)0xAA)
#define UART_FRAME_HEADER_1 ((uint8_t)0x55)
#define UART_FRAME_TAIL ((uint8_t)0x0D)

#define UART_MAX_FRAME_LEN 256

#define UART_PROTOCOL_VERSION ((uint8_t)0x03)

typedef enum {
    UART_CMD_SERVO_CONTROL = 0x01,
    UART_CMD_QUERY = 0x02,
    UART_CMD_INIT_HANDSHAKE = 0x10,
    UART_CMD_SERVO_STATE = 0x81,
    UART_CMD_SERVO_STATE_V2 = 0x82,
    UART_CMD_SYSTEM_STATE = 0x83,
    UART_CMD_EMERGENCY_STOP = 0xFF
} UartCmdId;

typedef enum {
    UART_SYSTEM_STATE_BOOT_CENTERING = 0x01,
    UART_SYSTEM_STATE_WAITING_CONNECTION = 0x02,
    UART_SYSTEM_STATE_ACTIVE = 0x03,
    UART_SYSTEM_STATE_ERROR = 0x7F
} UartSystemState;

typedef struct __attribute__((packed)) {
    uint8_t servo_id;
    int16_t angle_x10;
    uint16_t duration_ms;
} ServoCmdItem;

typedef struct __attribute__((packed)) {
    uint8_t servo_id;
    int16_t current_angle_x10;
    uint8_t status;
} ServoStateItem;

typedef struct __attribute__((packed)) {
    uint8_t servo_id;
    int16_t current_angle_x10;
    uint8_t status;
    uint16_t timestamp_ms;
    uint16_t frame_seq;
} ServoStateItem_v2;

typedef struct __attribute__((packed)) {
    uint8_t protocol_version;
    uint8_t requested_state;
    uint16_t reserved;
} UartHandshakePayload;

typedef struct __attribute__((packed)) {
    uint8_t protocol_version;
    uint8_t system_state;
    uint16_t reserved;
    uint32_t uptime_ms;
} UartSystemStatePayload;

uint16_t crc16_ccitt(const uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
