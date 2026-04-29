#ifndef SHARED__UART_PROTOCOL_H_
#define SHARED__UART_PROTOCOL_H_

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
    kUartCmdServoControl = 0x01,
    kUartCmdQuery = 0x02,
    kUartCmdInitHandshake = 0x10,
    kUartCmdServoState = 0x81,
    kUartCmdServoStateV2 = 0x82,
    kUartCmdSystemState = 0x83,
    kUartCmdEmergencyStop = 0xFF
} UartCmdId;

typedef enum {
    kUartSystemStateBootCentering = 0x01,
    kUartSystemStateWaitingConnection = 0x02,
    kUartSystemStateActive = 0x03,
    kUartSystemStateError = 0x7F
} UartSystemState;

typedef struct __attribute__((packed)) {
    uint8_t servo_id;      /* 0=front_left, 1=front_right, 2=rear_left, 3=rear_right */
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
} ServoStateItemV2;

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

#endif  /* SHARED__UART_PROTOCOL_H_ */
