# Raspberry Pi 快速启动

## 1. 准备 ROS 2 环境

```bash
source /opt/ros/jazzy/setup.bash
source ~/ros2_ws/install/setup.bash
cd ~/desktop_tracking_robot
export ROBOT_DDS_ROLE=rpi
export ROBOT_WSL_IP=<WSL_IP>
source scripts/ros2_network_env.sh
```

## 2. 启动相机推流

```bash
./scripts/rpi_start_camera.sh <WSL_IP> 5600
```

## 3. 启动串口桥

```bash
./scripts/rpi_start_ros.sh
```

## 4. 诊断命令

```bash
ls -l /dev/video0
ls -l /dev/ttyAMA0
ros2 node list
ros2 topic list
```

## 5. 常见现象

- `uart_bridge_node` 如果一直提示握手未完成，先检查 STM32 是否已进入 `WAITING_CONNECTION`
- 相机没有出流时，先确认 `camera.sh` 是否需要额外启用双目模式
