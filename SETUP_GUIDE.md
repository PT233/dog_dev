# 设置指南

本文档描述当前代码树对应的环境准备、构建和部署流程。

## 1. 目标运行形态

- `WSL2 / PC`: 运行视觉主链
- `Raspberry Pi`: 运行相机推流和 `uart_bridge`
- `STM32F103`: 运行四舵机实时控制固件

## 2. 目录和关键入口

- ROS 2 工作空间：`ros2_ws/`
- Raspberry Pi 启动脚本：`scripts/rpi_start_camera.sh`、`scripts/rpi_start_ros.sh`
- 一键部署：`scripts/rpi_one_click_deploy.sh`
- 交叉编译：`scripts/rpi_cross_build.sh`
- 网络配置：`scripts/ros2_network_env.sh`
- 固件工程：`stm32_keil/STM32.uvprojx`

## 3. WSL2 / PC 环境

### 必需项

- Ubuntu 24.04 on WSL2
- ROS 2 Jazzy
- OpenCV、GStreamer、colcon
- NVIDIA 驱动与 CUDA 可用
- ONNX Runtime SDK，并设置 `ONNXRUNTIME_ROOT`

### 构建步骤

```bash
source /opt/ros/jazzy/setup.bash
export ONNXRUNTIME_ROOT=/path/to/onnxruntime
cd /home/peter/dog/dog_dev/ros2_ws
colcon build
source install/setup.bash
```

### 网络环境

```bash
cd /home/peter/dog/dog_dev
export ROBOT_DDS_ROLE=wsl
source scripts/ros2_network_env.sh
```

默认效果：

- `ROS_DOMAIN_ID=42`
- `RMW_IMPLEMENTATION=rmw_cyclonedds_cpp`
- 生成只绑定单一网卡、只使用单播 peers 的 CycloneDDS 配置

## 4. Raspberry Pi 环境

### 必需项

- Ubuntu 24.04 ARM64
- ROS 2 Jazzy
- `colcon`
- `gst-launch-1.0`
- 摄像头可见为 `/dev/video0`
- UART 可见为 `/dev/ttyAMA0`

### 推荐部署方式：一键部署

```bash
cd /home/peter/dog/dog_dev
scripts/rpi_one_click_deploy.sh --pi ubuntu@192.168.137.100
```

默认行为：

- 同步 `robot_interfaces`、`uart_bridge`、`robot_bringup`
- 同步 Pi 端脚本到 `/home/ubuntu/desktop_tracking_robot/scripts`
- 在 Pi 上构建 ROS 2 包
- 写入 ROS 2 网络环境到 Pi 用户的 `.bashrc`
- 默认安装并启用开机服务

### 交叉编译后部署

```bash
export RPI_SYSROOT=$HOME/rpi-sysroot
cd /home/peter/dog/dog_dev
scripts/rpi_cross_build.sh
scripts/rpi_one_click_deploy.sh \
  --prebuilt-tar deploy/ros2_ws_install_rpi_aarch64.tgz
```

`rpi_cross_build.sh` 只为树莓派侧的三个包构建 ARM64 安装产物：

- `robot_interfaces`
- `uart_bridge`
- `robot_bringup`

### Pi 上手动构建

```bash
source /opt/ros/jazzy/setup.bash
cd ~/ros2_ws
colcon build --merge-install \
  --packages-select robot_interfaces uart_bridge robot_bringup
source install/setup.bash
```

### Pi 网络环境

```bash
cd /home/ubuntu/desktop_tracking_robot
export ROBOT_DDS_ROLE=rpi
export ROBOT_WSL_IP=<WSL_IP>
source scripts/ros2_network_env.sh
```

## 5. STM32 固件

### 工程位置

- 工程文件：`stm32_keil/STM32.uvprojx`
- CubeMX 配置：`stm32_keil/STM32.ioc`
- 启动与 RTOS：`stm32_keil/Core/Src/main.c`、`freertos.c`

### 关键源码

- `Core/Src/uart_rx_task.c`
- `Core/Src/traj_planner.c`
- `Core/Src/status_safety_task.c`
- `Core/Src/servo_driver.c`

### 共享协议

ROS 侧与 STM32 侧共用协议定义：

- 规范源：`shared/uart_protocol.h`
- STM32 镜像：`stm32_keil/Core/Inc/uart_protocol.h`

更新协议时，两边必须同步。

## 6. 接线要点

详细接线见 [hardware_wiring.html](hardware_wiring.html)。

最关键的几项：

- 树莓派 UART 对接 STM32 `USART1`
- SG90 四路连接 STM32 `PA0 ~ PA3`
- 舵机使用独立 `5V` 供电
- 树莓派 USB 摄像头为 `/dev/video0`

## 7. 首次联调建议顺序

1. 先在 WSL2 完成 `ros2_ws` 构建
2. 单独验证 `scripts/ros2_network_env.sh`
3. 用 `rpi_one_click_deploy.sh` 把 Pi 侧 ROS 包部署好
4. 单独验证 Pi 视频推流
5. 单独验证 Pi `uart_bridge` 能打开 `/dev/ttyAMA0`
6. 最后联调 STM32 握手和 `/servo_state`

## 8. 常见坑

- `detection_node` 没有 `ONNXRUNTIME_ROOT` 时无法构建
- WSL2 和树莓派如果没有统一执行 `ros2_network_env.sh`，DDS 常常表现为可 ping 通但互相看不到节点
- `uart_bridge_node` 在握手完成前会主动丢弃 `/servo_cmd`
- `CalibrateCenter.srv` 目前没有服务端，不要把它当成现成功能
