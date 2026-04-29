#include "uart_bridge/uart_protocol.h"

/* ROS 2 侧与 STM32 侧共用同一套 CRC16-CCITT 规则。
 * 计算范围是 CMD_ID + LEN + PAYLOAD，不包含帧头和帧尾。
 */
uint16_t crc16_ccitt(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFFu;

    if (len == 0u) {
        return crc;
    }

    if (data == NULL) {
        return 0u;
    }

    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (uint8_t bit = 0; bit < 8u; ++bit) {
            /* 多项式 0x1021；最高位为 1 时左移后异或多项式。 */
            if ((crc & 0x8000u) != 0u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }

    return crc;
}
