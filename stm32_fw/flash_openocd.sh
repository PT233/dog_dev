#!/bin/bash

# OpenOCD SWD 烧录脚本
# 需要：openocd, arm-none-eabi-toolchain, J-Link

set -e

cd "$(dirname "$0")"

# 编译
echo "[1/3] 编译固件..."
make -j4 > /dev/null 2>&1

# 烧录
echo "[2/3] 烧录固件 (SWD 模式)..."

timeout 30 openocd \
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

if [ $? -eq 0 ]; then
    echo "[3/3] ✓ 烧录成功"
else
    echo "[3/3] ✗ 烧录失败"
    exit 1
fi
