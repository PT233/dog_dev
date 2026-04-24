#!/bin/bash
set -e

RESET='\033[0m'
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'

echo -e "${BLUE}========== STM32F103CB 固件烧录 (SWD 模式) ==========${RESET}"
echo ""

cd "$(dirname "$0")"

# 1. 编译固件
echo -e "${YELLOW}[1/3] 编译固件...${RESET}"

MAKE_CMD="make"
if ! command -v make &> /dev/null; then
    if command -v mingw32-make &> /dev/null; then
        MAKE_CMD="mingw32-make"
    else
        echo -e "${RED}✗ 编译工具不可用（需要 make 或 mingw32-make）${RESET}"
        exit 1
    fi
fi

if $MAKE_CMD clean > /dev/null 2>&1; then
    :
fi

if $MAKE_CMD -j4 > /dev/null 2>&1; then
    echo -e "${GREEN}✓ 编译成功${RESET}"
else
    echo -e "${RED}✗ 编译失败${RESET}"
    exit 1
fi

# 2. 验证 J-Link 连接
echo -e "${YELLOW}[2/3] 验证 J-Link 连接...${RESET}"

if lsusb | grep -q "1366:"; then
    echo -e "${GREEN}✓ J-Link 已连接 (SWD 模式)${RESET}"
else
    echo -e "${RED}✗ 错误：J-Link 未连接或未透传${RESET}"
    echo ""
    echo -e "${YELLOW}Windows 端需要运行以下命令重新透传：${RESET}"
    echo "  PS> usbipd list"
    echo "  PS> usbipd detach --busid 5-4"
    echo "  PS> usbipd attach --wsl default --busid 5-4"
    exit 1
fi

# 3. 烧录
echo -e "${YELLOW}[3/3] 烧录固件到 0x08000000 (SWD)...${RESET}"
echo ""

timeout 60 openocd \
    -c "adapter driver jlink" \
    -c "transport select swd" \
    -c "adapter speed 1000" \
    -c "set CHIPNAME stm32f103" \
    -c "set CPUTAPID 0x3ba00477" \
    -c "source [find target/stm32f1x.cfg]" \
    -c "init" \
    -c "reset halt" \
    -c "flash erase_sector 0 0 63" \
    -c "program build/STM32.bin verify 0x08000000" \
    -c "reset run" \
    -c "shutdown" 2>&1

RESULT=$?

if [ $RESULT -eq 0 ]; then
    echo ""
    echo -e "${BLUE}========== 烧录完成 ==========${RESET}"
    echo ""
    echo -e "${GREEN}✓ 固件已成功烧录${RESET}"
    echo ""
    echo -e "${YELLOW}下一步：验证串口输出${RESET}"
    echo "  1. 树莓派端打开串口监听："
    echo "     ssh ubuntu@192.168.137.100"
    echo "     minicom -D /dev/ttyAMA0 -b 921600"
    echo ""
    echo "  2. 应该每 50ms 看到一条状态输出："
    echo "     [STATUS] Servo0: 90°, Servo1: 90°, Servo2: 90°, Servo3: 90°"
else
    echo ""
    echo -e "${RED}✗ 烧录失败${RESET}"
    echo ""
    echo -e "${YELLOW}故障排查：${RESET}"
    echo "  1. 检查接线 (SWD 模式)："
    echo "     J-Link Pin 3  (GND)   → STM32 GND"
    echo "     J-Link Pin 7  (SWDIO) → STM32 PA13"
    echo "     J-Link Pin 9  (SWCLK) → STM32 PA14"
    echo ""
    echo "  2. 重新透传 J-Link (Windows PowerShell)："
    echo "     usbipd detach --busid 5-4"
    echo "     usbipd attach --wsl default --busid 5-4"
    echo ""
    echo "  3. 重新运行此脚本"
    exit 1
fi
