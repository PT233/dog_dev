# 物体识别跟随机器人 - 完整设置指南

**最后更新**：2026-04-24  
**项目代号**：`desktop_tracking_robot` v3.0  
**目标**：桌面玩具机器人基于双目摄像头识别 COCO 80 类物体并用舵机持续跟随

---

## 目录

1. [硬件准备](#1-硬件准备)
2. [硬件接线](#2-硬件接线)
3. [软件环境搭建](#3-软件环境搭建)
4. [固件烧录](#4-固件烧录)
5. [系统调试](#5-系统调试)
6. [应用启动](#6-应用启动)
7. [PID 调参](#7-pid-调参)
8. [常见问题](#8-常见问题)

---

## 1. 硬件准备

### 1.1 物料清单

| 设备 | 数量 | 规格 | 用途 |
|---|---|---|---|
| PC（Windows 11）| 1 | WSL2 + Ubuntu 24.04 | 视觉处理和决策 |
| 树莓派 4B | 1 | 2GB RAM + 4GB Swap | 相机接入和协议桥接 |
| STM32F103C8T6 | 1 | 64KB Flash / 20KB RAM | 实时舵机控制 |
| USB 双目摄像头 | 1 | 640×480 @30fps | 视觉输入 |
| SG90 舵机 | 4 | 5V, 0.5~2.5ms 脉宽 | Yaw(1) + Pitch(1) + 预留(2) |
| MPU6050 | 1 | I2C 接口 | 姿态传感器（可选） |
| USB-TTL 模块 | 1 | 3.3V 逻辑 | 调试串口 |
| 网线 | 1 | RJ45 | PC ↔ 树莓派 |
| USB 线 | 2 | Type-A to Micro-B | STM32 烧录 + 树莓派电源 |
| 面包板 + 杜邦线 | 若干 | - | 连接 |

### 1.2 环境要求

#### PC 端
- **Windows 11** + **WSL2** + **Ubuntu 24.04**
- **NVIDIA GPU**（推荐 RTX 3060 或更新）+ CUDA 12.1
- **ROS 2 Jazzy**（已安装在 WSL2）
- **Python 3.10+** + ultralytics（YOLOv8）
- **GStreamer 1.0**（视频推流）
- **CMake 3.20+**、**colcon** 构建工具

#### 树莓派端
- **Raspberry Pi 4B**（2GB 以上 RAM）
- **Ubuntu Server 24.04 ARM64**
- **ROS 2 Jazzy**
- **USB 双目摄像头**驱动（V4L2）
- **GStreamer 1.0**（硬件 H.264 编码）

#### STM32 端
- **STM32CubeIDE** 或 **CubeF1** + **GNU Arm Embedded Toolchain**
- **FreeRTOS** CMSIS V2 接口
- **UART DMA** + **PWM** 配置

---

## 2. 硬件接线

### 2.1 连接拓扑

```
┌─────────────────────────────────────────────────────────┐
│                     PC (WSL2)                           │
│               视觉处理 + 决策控制                        │
│          ROS 2 DDS (UDP 5600 + mirrored)              │
└──────────────────────┬──────────────────────────────────┘
                       │ 网线直连 (192.168.137.0/24)
                       │
        ┌──────────────▼───────────────┐
        │    树莓派 4B                   │
        │  ROS 2 节点 + GStreamer       │
        │  192.168.137.100             │
        └──────────────┬───────────────┘
                       │ GPIO UART (TXD/RXD)
                       │ 921600 bps + CRC16
                       │
        ┌──────────────▼───────────────┐
        │   STM32F103C8T6               │
        │  FreeRTOS 舵机控制器          │
        │  PWM 50Hz + UART 通信         │
        └──────────────┬───────────────┘
                       │ PWM (PA0~PA3)
            ┌──────────┼──────────┐
            │          │          │
      ┌─────▼─┐  ┌──────▼──┐  ┌─▼────────┐
      │ SG90  │  │  SG90   │  │预留(2路) │
      │ Yaw   │  │ Pitch   │  │          │
      └───────┘  └─────────┘  └──────────┘
```

### 2.2 详细接线图

**→ 请查看 `hardware_wiring.html` 获取交互式接线图**

### 2.3 接线清单

#### STM32 接线

| STM32 引脚 | 功能 | 外设 | 备注 |
|---|---|---|---|
| PA0 | TIM2_CH1 | SG90 Yaw | PWM 输出 |
| PA1 | TIM2_CH2 | SG90 Pitch | PWM 输出 |
| PA2 | TIM2_CH3 | 预留 | PWM 输出 |
| PA3 | TIM2_CH4 | 预留 | PWM 输出 |
| PA9 | USART1_TX | 树莓派 RXD | 波特率 921600 |
| PA10 | USART1_RX | 树莓派 TXD | 波特率 921600 |
| PB12/PB13 | I2C2 | MPU6050 (可选) | I2C 地址 0x68 |
| GND | 地线 | - | **必须连接** |
| 3.3V | 电源 | - | 仅供逻辑电源 |

#### 树莓派接线

| 树莓派引脚 | 功能 | 外设 | 备注 |
|---|---|---|---|
| GPIO17 (TXD) | UART TX | STM32 PA10 | 3.3V 逻辑 |
| GPIO27 (RXD) | UART RX | STM32 PA9 | 3.3V 逻辑 |
| GPIO2 (SDA) | I2C SDA | MPU6050 (可选) | 上拉 4.7kΩ |
| GPIO3 (SCL) | I2C SCL | MPU6050 (可选) | 上拉 4.7kΩ |
| USB | 相机输入 | USB 双目摄像头 | /dev/video0 |
| GND | 地线 | STM32 GND | **必须连接** |

#### 电源分配

| 设备 | 电压 | 电流 | 电源来源 |
|---|---|---|---|
| STM32 逻辑 | 3.3V | ~50mA | 树莓派或 USB |
| SG90 舵机 ×4 | 5V | ~200mA（加载） | **独立 USB 电源** |
| MPU6050 | 3.3V | ~5mA | 树莓派 3.3V |
| 树莓派 | 5V | ~500mA | USB Type-C |

⚠️ **重要**：舵机必须使用**独立 5V 电源**，不能从树莓派 USB 供电

---

## 3. 软件环境搭建

### 3.1 WSL2 环境配置

```bash
# 1. 确认 CUDA 可用
nvidia-smi

# 2. 安装 ROS 2 Jazzy（如未安装）
sudo apt update
sudo apt install ros-jazzy-desktop

# 3. 设置环境变量（添加到 ~/.bashrc）
cat >> ~/.bashrc << 'EOF'
# ROS 2 Setup
source /opt/ros/jazzy/setup.bash
export ROS_DOMAIN_ID=42
export ROS_LOCALHOST_ONLY=0
EOF

source ~/.bashrc

# 4. 验证 ROS 2
ros2 --version  # 应输出 Jazzy
```

### 3.2 树莓派环境配置

```bash
# 1. SSH 连接到树莓派
ssh ubuntu@192.168.137.100

# 2. 更新系统
sudo apt update && sudo apt upgrade -y

# 3. 配置 UART（禁用蓝牙）
sudo nano /boot/firmware/config.txt
# 添加以下行：
#   dtoverlay=disable-bt
#   enable_uart=1

sudo systemctl disable hciuart
sudo reboot

# 4. 安装 ROS 2 Jazzy
sudo apt install ros-jazzy-ros-core

# 5. 配置 ~/.bashrc
cat >> ~/.bashrc << 'EOF'
source /opt/ros/jazzy/setup.bash
export ROS_DOMAIN_ID=42
export ROS_LOCALHOST_ONLY=0
EOF

source ~/.bashrc

# 6. 验证 UART
ls -l /dev/ttyAMA0  # 应存在
```

### 3.3 编译本项目代码

```bash
# WSL2
cd /home/peter/dog/dog_dev
colcon build --symlink-install

# 树莓派
scp -r /home/peter/dog/dog_dev/ros2_ws ubuntu@192.168.137.100:~
ssh ubuntu@192.168.137.100
cd ~/ros2_ws
colcon build --packages-select uart_bridge robot_interfaces
```

---

## 4. 固件烧录

### 4.1 STM32 CubeIDE 编译与烧录

```bash
# 1. 打开 STM32CubeIDE
# 2. 打开工程：desktop_tracking_robot/stm32_fw
# 3. 编译：Project → Build All
# 4. 烧录：Run → Run As → STM32 Application

# 或命令行编译
cd stm32_fw
mingw32-make -j4
```

### 4.2 验证固件

```bash
# 使用串口监控工具（如 minicom）连接 STM32
minicom -D /dev/ttyUSB0 -b 921600

# 应该看到以下日志（每 50ms）：
# [STATUS] Servo0: 90°, Servo1: 90°, ...
```

---

## 5. 系统调试

### 5.1 UART 通信验证

```bash
# 树莓派端测试脚本
python3 scripts/test_uart_rpi.py

# 输出应显示：
# ✓ Servo control command sent
# ✓ Status feedback received: id=0 angle=90.0
```

### 5.2 网络通信验证

```bash
# WSL2
export ROS_DOMAIN_ID=42
ros2 topic list  # 应列出所有话题

# 树莓派
export ROS_DOMAIN_ID=42
ros2 topic list  # 应与 WSL2 同步
```

### 5.3 相机验证

```bash
# 树莓派
v4l2-ctl --device=/dev/video0 --list-formats
# 应显示 YUYV 640×480@30fps

# 启动推流
./scripts/start_camera_stream.sh

# WSL2 验证接收
gst-launch-1.0 udpsrc port=5600 ! \
  "application/x-rtp,media=video,encoding-name=H264" ! \
  rtpjitterbuffer ! rtph264depay ! avdec_h264 ! \
  videoconvert ! autovideosink
```

---

## 6. 应用启动

### 6.1 启动顺序（严格按此顺序）

**Terminal 1 - 树莓派：启动相机推流**
```bash
ssh ubuntu@192.168.137.100
cd desktop_tracking_robot
export ROS_DOMAIN_ID=42
./scripts/start_camera_stream.sh
```

**Terminal 2 - 树莓派：启动树莓派节点**
```bash
ssh ubuntu@192.168.137.100
source ~/ros2_ws/install/setup.bash
export ROS_DOMAIN_ID=42
ros2 launch robot_bringup rpi_stack.launch.py
```

**Terminal 3 - WSL2：启动 WSL2 节点**
```bash
cd /home/peter/dog/dog_dev
source install/setup.bash
export ROS_DOMAIN_ID=42
ros2 launch robot_bringup vision_stack.launch.py
```

**Terminal 4 - WSL2：设置目标类别（可选）**
```bash
ros2 service call /set_target_class \
  robot_interfaces/srv/SetTargetClass "{class_name: 'person'}"
```

### 6.2 验证系统运行

```bash
# Terminal 5 - 监控话题
ros2 topic hz /tracked_objects     # 应显示 ~30Hz
ros2 topic echo /pixel_error       # 应显示实时误差
ros2 topic echo /servo_cmd         # 应显示舵机命令
```

---

## 7. PID 调参

### 7.1 参数位置

文件：`config/visual_servo.yaml`

```yaml
visual_servo_node:
  ros__parameters:
    yaw:
      kp: 0.05    # 比例增益
      ki: 0.001   # 积分增益
      kd: 0.02    # 微分增益
    pitch:
      kp: 0.04
      ki: 0.001
      kd: 0.02
```

### 7.2 调参步骤

1. **监控误差曲线**
   ```bash
   rqt_plot /pixel_error/x /pixel_error/y
   ```

2. **现象 → 调整**
   | 现象 | 调整 |
   |---|---|
   | 反应迟钝 | 增大 Kp |
   | 振荡/超冲 | 减小 Kp 或增大 Kd |
   | 稳态有偏 | 增大 Ki（小幅） |
   | 动作抖动 | 增大 Kd 或减小 Ki |

3. **动态加载参数**
   ```bash
   ros2 param set /visual_servo_node yaw.kp 0.06
   ```

### 7.3 调参结果记录

调好的参数应记录在 `docs/pid_tuning.md`（任务 7.6）

---

## 8. 常见问题

### Q1：树莓派无法与 STM32 通信

**症状**：`uart_bridge_node` 启动时报错：`Failed to open /dev/ttyAMA0`

**解决方案**：
```bash
# 1. 检查 UART 是否启用
ls -l /dev/ttyAMA0

# 2. 检查蓝牙是否禁用
sudo systemctl status hciuart

# 3. 重新配置
sudo nano /boot/firmware/config.txt
# 添加 dtoverlay=disable-bt 和 enable_uart=1
sudo reboot
```

### Q2：网络无法同步

**症状**：`ROS_DOMAIN_ID` 设置正确，但 `ros2 topic list` 在两端不同步

**解决方案**：
```bash
# 1. 确保两端的 ROS_DOMAIN_ID 完全相同
echo $ROS_DOMAIN_ID  # 应输出 42

# 2. 检查防火墙
sudo ufw status

# 3. 检查 mirrored 网络配置
ipconfig /all  # Windows，查找 WSL 虚拟交换机

# 4. 重启 ROS Daemon
pkill rmw_fastrtps_cpp
sleep 2
ros2 topic list
```

### Q3：舵机不动或动作抖动

**症状**：舵机收到命令但无反应，或不停抖动

**解决方案**：
1. **检查电源**：舵机是否有独立 5V 电源？（重要！）
2. **检查 PWM**：示波器量 PA0~PA3 脉宽是否在 0.5~2.5ms 范围？
3. **检查 CRC**：STM32 是否正确校验了串口帧？
4. **减小 Kp**：PID 增益过大会导致振荡

### Q4：相机画面卡顿或延迟大

**症状**：`rqt_image_view` 显示的画面明显延迟

**解决方案**：
1. **检查网络**：`iperf3` 测试 PC ↔ 树莓派 带宽
2. **检查 CPU**：树莓派上运行 `htop` 查看负载
3. **调整 GStreamer**：降低分辨率或帧率
4. **检查 UDP 丢包**：`tcpdump -i eth0 udp port 5600`

### Q5：YOLOv8 推理很慢（>100ms）

**症状**：`detection_node` 的推理延迟太高

**解决方案**：
1. **确认 GPU 使用**：`nvidia-smi` 是否显示 Python 进程？
2. **预热模型**：第一次推理会加载模型，耗时较长
3. **减小输入尺寸**：在 `detection.yaml` 中改小 `input_size`
4. **使用 TensorRT**：将 ONNX 转换为 TensorRT 加速（需要额外配置）

---

## 🔗 相关文档

- **硬件接线**：`hardware_wiring.html`
- **架构设计**：`architecture.md`
- **任务清单**：`task.md`
- **调参记录**：`docs/pid_tuning.md`（完成后）

---

**最后更新**：2026-04-24  
**作者**：Claude AI  
**许可证**：MIT
