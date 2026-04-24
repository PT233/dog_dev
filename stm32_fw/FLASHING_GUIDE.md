# STM32F103CB J-Link 烧录指南

**硬件**：J-Link 调试器 + STM32F103CB 微控制器

---

## 🔌 硬件接线

```
J-Link          STM32F103CB
──────────────────────────
GND       ──>   GND
SWDIO     ──>   PA13
SWCLK     ──>   PA14
3.3V      ──>   3.3V（可选，如果 J-Link 不给电）
```

**连接检查**：
- [ ] J-Link 通过 USB 连接到 Windows
- [ ] SWDIO、SWCLK、GND 正确连接到 STM32
- [ ] STM32 有电源供应（5V 或 3.3V）

---

## 🚀 快速烧录（3 步）

### Step 1：Windows 端 - 透传 J-Link 给 WSL2

在 Windows **PowerShell（管理员）** 中运行：

```powershell
# 方式 A：使用自动化脚本（推荐）
cd C:\Users\Pt\Desktop\dog\agent_dev\stm32_fw
.\flash_setup_windows.ps1

# 方式 B：手动命令
usbipd list                      # 找 J-Link 的 BUSID
usbipd attach --wsl default --busid <BUSID>
```

**预期输出**：
```
BUSID  VID:PID      DEVICE
1-1    1366:1015    SEGGER J-Link
✓ 已透传给 WSL2
```

### Step 2：WSL2 端 - 验证 J-Link 连接

在 **WSL2 Bash** 中运行：

```bash
lsusb | grep Segger
# 输出：Bus 001 Device 002: ID 1366:1015 Segger J-Link
```

### Step 3：WSL2 端 - 烧录固件

在 **WSL2 Bash** 中运行：

```bash
cd /home/peter/dog/dog_dev/stm32_fw
./flash.sh
```

**预期输出**：
```
========== STM32F103CB 固件烧录 ==========

[1/3] 编译固件...
✓ 编译成功
[2/3] 验证 J-Link 连接...
✓ J-Link 已连接
[3/3] 烧录固件到 0x08000000...

Device name: STM32F103CB
Erase flash... OK
Downloading file [build/STM32.bin]... OK
Verify flash... OK
Resetting target...

========== 烧录完成 ==========

✓ 固件已成功烧录
```

---

## ✅ 验证烧录成功

在树莓派上连接串口监听（921600 波特率）：

```bash
# 树莓派
ssh ubuntu@192.168.137.100
minicom -D /dev/ttyAMA0 -b 921600

# 应该看到（每 50ms 输出一行）：
# [STATUS] Servo0: 90°, Servo1: 90°, Servo2: 90°, Servo3: 90°
# [STATUS] Servo0: 90°, Servo1: 90°, Servo2: 90°, Servo3: 90°
# ...

# 按 Ctrl+A 再 X 退出 minicom
```

---

## 🔧 单独命令（进阶）

### 仅编译，不烧录

```bash
cd stm32_fw
mingw32-make -j4
# 输出：build/STM32.hex, build/STM32.bin, build/STM32.elf
```

### 仅烧录，不编译

```bash
cd stm32_fw
jlink -commandfile flash_jlink.jlink
```

### 手动交互式烧录

```bash
jlink
# 交互式命令行，输入：
usb
device STM32F103CB
connect
erase
loadbin build/STM32.bin 0x08000000
verifybin build/STM32.bin 0x08000000
reset
exit
```

---

## ❌ 故障排查

### 问题 1：J-Link 未连接或未透传

**错误信息**：
```
✗ 错误：J-Link 未连接或未透传
```

**解决**：
```powershell
# Windows PowerShell (管理员)
usbipd list                       # 找 J-Link BUSID
usbipd attach --wsl default --busid 1-1
```

### 问题 2：编译失败

**错误信息**：
```
✗ 编译失败
```

**解决**：
```bash
cd stm32_fw
mingw32-make clean
mingw32-make -j4
# 如果仍失败，检查是否安装 ARM 工具链
arm-none-eabi-gcc --version
```

### 问题 3：设备未响应

**错误信息**：
```
Device not responding
```

**解决**：
1. 检查硬件接线
   - SWDIO (J-Link TCO/TDI) ↔ STM32 PA13
   - SWCLK (J-Link TCK) ↔ STM32 PA14
   - GND ↔ GND
   - 3.3V ↔ 3.3V（可选）

2. 重新烧录：
   ```bash
   jlink
   # 输入：
   usb
   device STM32F103CB
   connect
   # 如果卡住，按 Ctrl+C，检查接线后重试
   ```

### 问题 4：烧录失败或验证失败

**错误信息**：
```
Flash write failed
Verification failed
```

**解决**：
```bash
# 尝试完全擦除并重新烧录
jlink
usb
device STM32F103CB
connect
erase                     # 完全擦除 Flash
loadbin build/STM32.bin 0x08000000
verifybin build/STM32.bin 0x08000000
reset
exit
```

### 问题 5：串口无输出

**错误**：烧录成功，但树莓派的 minicom 没有看到 `[STATUS]` 输出

**原因和解决**：
1. UART 接线错误
   - STM32 PA9 (TX) → RPI GPIO14 (RX)
   - STM32 PA10 (RX) → RPI GPIO15 (TX)
   - GND → GND

2. 树莓派串口权限
   ```bash
   ssh ubuntu@192.168.137.100
   sudo usermod -aG dialout ubuntu
   # 需要重新登录或重启树莓派
   ```

3. 波特率不匹配
   ```bash
   # 确保都是 921600
   minicom -D /dev/ttyAMA0 -b 921600
   ```

---

## 📋 烧录流程检查清单

- [ ] Windows：J-Link 插入 USB
- [ ] Windows：打开 PowerShell（管理员）运行 `./flash_setup_windows.ps1`
- [ ] WSL2：验证 `lsusb | grep Segger`
- [ ] WSL2：运行 `./flash.sh`
- [ ] 看到 `✓ 固件已成功烧录`
- [ ] 树莓派：看到 `[STATUS]` 串口输出

---

## 📞 更多帮助

查看主项目文档：
- **SETUP_GUIDE.md**：完整的硬件和软件设置指南
- **QUICK_START.md**：快速启动命令参考
- **hardware_wiring.html**：交互式硬件接线图

---

**Last Updated**: 2026-04-24
