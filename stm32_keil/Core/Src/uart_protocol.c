#include "uart_protocol.h"

/* 计算 UART 帧校验值。
 *
 * 调用方只传入 CMD_ID + LEN + PAYLOAD 这一段数据，帧头和帧尾不参与 CRC。
 * 返回约定：
 * - len == 0 时返回初始值 0xFFFF，用于兼容空 payload 帧；
 * - data == NULL 且 len > 0 时返回 0，显式标记调用错误。
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
            /* 最高位为 1 时左移后异或多项式 0x1021；否则只左移一位。 */
            if ((crc & 0x8000u) != 0u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }

    return crc;
}
