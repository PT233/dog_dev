# 任务 2.1 执行指南：树莓派 UART 物理连通性测试

## 前置条件
- ✅ STM32 已烧录任务 1.5 固件
- ✅ GPIO UART 硬件连接（树莓派 RXD → STM32 TX；树莓派 TXD → STM32 RX）
- ✅ STM32 和树莓派电源已接通

## 步骤 1：修改树莓派系统配置

在树莓派上执行：

```bash
# 编辑 /boot/firmware/config.txt
sudo nano /boot/firmware/config.txt
```

在文件末尾加入：
```
# Enable UART and disable Bluetooth
dtoverlay=disable-bt
enable_uart=1
```

保存并退出（Ctrl+O, Enter, Ctrl+X）。

## 步骤 2：禁用 hciuart 服务

```bash
sudo systemctl disable hciuart
```

## 步骤 3：重启树莓派

```bash
sudo reboot
```

等待约 30 秒重启完成。

## 步骤 4：验证 UART 设备存在

重启后，确认设备可用：

```bash
ls -l /dev/ttyAMA0
```

应该看到类似输出：
```
crw-rw---- 1 root dialout 204, 64 Apr 23 10:00 /dev/ttyAMA0
```

## 步骤 5：安装 Python pyserial（如未安装）

```bash
pip3 install pyserial
```

## 步骤 6：运行测试脚本

```bash
# 将脚本文件放到树莓派上（或通过 git clone 获取）
# 然后运行：
python3 scripts/test_uart_rpi.py
```

### 预期输出示例

```
============================================================
树莓派 UART 连通性测试 (任务2.1)
============================================================
端口: /dev/ttyAMA0
波特率: 921600 bps
超时: 1.0 s

✅ 成功打开 /dev/ttyAMA0

--- 测试 1：发送舵机控制帧 ---
命令：servo 0 转到 90°，耗时 1000ms
  servo_id=0, angle=900 (90.0°), duration=1000ms
  帧内容：AA 55 01 05 00 84 03 E8 03 A7 99 0D
  帧长：12 字节
  ✅ 已发送 12 字节

--- 测试 2：接收状态反馈帧（20 秒内）---
期望：STM32 每 50ms 发一次状态帧（CMD_ID=0x81）

[10:00:05] ✅ 帧 #1： servo0=0.0° (idle) servo1=90.0° (idle) servo2=90.0° (idle) servo3=90.0° (idle)
[10:00:05] ✅ 帧 #2： servo0=5.0° (moving) servo1=90.0° (idle) servo2=90.0° (idle) servo3=90.0° (idle)
[10:00:05] ✅ 帧 #3： servo0=10.0° (moving) servo1=90.0° (idle) servo2=90.0° (idle) servo3=90.0° (idle)
...（后续每 50ms 一条）...

============================================================
测试结果汇总
============================================================
✅ 成功接收帧数：20
⚠️  坏帧数：0

✅ 验收通过！STM32 与树莓派 UART 连通正常。
   - 舵机应该在这 20 秒内转到 90° 并维持
   - 物理上应该听到舵机齿轮转动声
```

## 验收标准

✅ **任务通过条件**：
1. 脚本执行后 STM32 舵机 0 能动（从 0° 转到 90°）
2. 脚本能连续接收到至少 3 个状态反馈帧（CMD_ID=0x81）
3. 显示 "✅ 验收通过"

## 故障排查

| 问题 | 解决方案 |
|-----|--------|
| `找不到 /dev/ttyAMA0` | 确保已 reboot，检查 /boot/firmware/config.txt 是否保存成功 |
| 舵机不动 | 检查 GPIO UART 接线（RXD/TXD 是否交叉），确认 STM32 已烧录并上电 |
| 接收不到状态帧 | 确认 STM32 固件中 Task_Status_TX 已启动（在 freertos.c） |
| CRC 错误 | 表示有噪声或波特率不匹配，检查接线质量和硬件流控 |

## 下一步

任务 2.1 通过后，进入**任务 2.2：创建 ros2_ws 工作空间和 robot_interfaces 包**。
