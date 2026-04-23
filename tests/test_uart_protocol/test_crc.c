#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "uart_protocol.h"

static int check_crc(const char* name, const uint8_t* data, size_t len, uint16_t expected) {
    uint16_t actual = crc16_ccitt(data, len);
    if (actual != expected) {
        printf("FAIL: %s expected=0x%04X actual=0x%04X\n", name, expected, actual);
        return 1;
    }
    printf("PASS: %s\n", name);
    return 0;
}

int main(void) {
    int failures = 0;

    static const uint8_t empty_data[1] = {0x00};
    failures += check_crc("empty input", empty_data, 0u, 0xFFFFu);

    static const uint8_t single_zero[] = {0x00};
    failures += check_crc("single byte 0x00", single_zero, sizeof(single_zero), 0xE1F0u);

    static const uint8_t known_text[] = "123456789";
    failures += check_crc("known vector 123456789", known_text, strlen((const char*)known_text), 0x29B1u);

    static const uint8_t random_16[] = {
        0x10, 0x23, 0x45, 0x67,
        0x89, 0xAB, 0xCD, 0xEF,
        0x55, 0xAA, 0x00, 0x11,
        0x22, 0x33, 0x44, 0x99
    };
    failures += check_crc("random 16 bytes", random_16, sizeof(random_16), 0x232Fu);

    static const uint8_t full_frame[] = {
        UART_FRAME_HEADER_0,
        UART_FRAME_HEADER_1,
        UART_CMD_SERVO_CONTROL,
        0x05,
        0x00,
        0x84,
        0x03,
        0xE8,
        0x03,
        0x4F,
        0xED,
        UART_FRAME_TAIL
    };
    uint16_t frame_crc_expected = (uint16_t)((uint16_t)full_frame[10] << 8) | full_frame[9];
    failures += check_crc("protocol full frame", &full_frame[2], 7u, frame_crc_expected);

    if (failures != 0) {
        printf("FAIL\n");
        return 1;
    }

    printf("PASS\n");
    return 0;
}
