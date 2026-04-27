#include <stdio.h>
#include "uart_protocol.h"
int main(void) {
    if ((sizeof(ServoCmdItem) == 5U) &&
        (sizeof(UartHandshakePayload) == 4U) &&
        (sizeof(UartSystemStatePayload) == 8U)) {
        puts("PASS");
        return 0;
    }
    puts("FAIL");
    return 1;
}
