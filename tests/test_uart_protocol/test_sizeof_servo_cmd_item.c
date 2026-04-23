#include <stdio.h>
#include "uart_protocol.h"
int main(void) {
    if (sizeof(ServoCmdItem) == 5U) {
        puts("PASS");
        return 0;
    }
    puts("FAIL");
    return 1;
}