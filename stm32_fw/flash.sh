#!/bin/bash
set -e

RESET='\033[0m'
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'

echo -e "${BLUE}========== STM32F103CB 固件烧录 ==========${RESET}"
echo ""

# 1. 编译固件
echo -e "${YELLOW}[1/3] 编译固件...${RESET}"
if mingw32-make clean > /dev/null 2>&1; then
    :
fi

if mingw32-make -j4 > /dev/null 2>&1; then
    echo -e "${GREEN}✓ 编译成功${RESET}"
else
    echo -e "${RED}✗ 编译失败${RESET}"
    exit 1
fi

# 2. 验证 J-Link 连接
echo -e "${YELLOW}[2/3] 验证 J-Link 连接...${RESET}"

if ! command -v lsusb &> /dev/null; then
    echo -e "${RED}✗ lsusb 未安装，无法验证 J-Link${RESET}"
    echo "   请运行：sudo apt-get install usbutils"
    exit 1
fi

if lsusb | grep -q "1366:1015"; then
    echo -e "${GREEN}✓ J-Link 已连接${RESET}"
else
    echo -e "${RED}✗ 错误：J-Link 未连接或未透传${RESET}"
    echo ""
    echo -e "${YELLOW}Windows 端需要运行：${RESET}"
    echo "  1. 打开 PowerShell (管理员)"
    echo "  2. 列出USB设备："
    echo "     usbipd list"
    echo "  3. 找到 J-Link (SEGGER, VID:PID 1366:1015)"
    echo "  4. 透传给 WSL2（BUSID 替换为实际值）："
    echo "     usbipd attach --wsl default --busid <BUSID>"
    echo ""
    echo "  之后在 WSL2 中重新运行此脚本"
    exit 1
fi

# 3. 烧录
echo -e "${YELLOW}[3/3] 烧录固件到 0x08000000...${RESET}"
echo ""

if jlink -commandfile flash_jlink.jlink; then
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
    echo ""
    echo "  3. 按 Ctrl+A 再 X 退出 minicom"
    echo ""
else
    echo ""
    echo -e "${RED}✗ 烧录失败${RESET}"
    echo ""
    echo -e "${YELLOW}故障排查：${RESET}"
    echo "  1. 检查接线："
    echo "     J-Link GND → STM32 GND"
    echo "     J-Link SWDIO (TCO/TDI) → STM32 PA13"
    echo "     J-Link SWCLK (TCK) → STM32 PA14"
    echo ""
    echo "  2. 重新透传 J-Link："
    echo "     Windows PowerShell: usbipd attach --wsl default --busid 1-1"
    echo ""
    echo "  3. 重新运行脚本"
    exit 1
fi
