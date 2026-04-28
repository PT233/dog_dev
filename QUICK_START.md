# 快速开始

适用前提：

- `ros2_ws` 已构建完成
- Raspberry Pi 已部署 `robot_interfaces`、`uart_bridge`、`robot_bringup`
- STM32 固件已烧录

## 1. WSL2 准备

```bash
source /opt/ros/jazzy/setup.bash
cd /home/peter/dog/dog_dev
source ros2_ws/install/setup.bash
export ROBOT_DDS_ROLE=wsl
source scripts/ros2_network_env.sh
```

## 2. Raspberry Pi 启动相机推流

```bash
cd /home/ubuntu/desktop_tracking_robot
./scripts/rpi_start_camera.sh <WSL_IP> 5600
```

如果你的相机启用脚本不是默认的 `$HOME/camera.sh`，先设置：

```bash
export CAMERA_ENABLE_SCRIPT=/path/to/camera.sh
```

## 3. Raspberry Pi 启动 ROS 2

```bash
cd /home/ubuntu/desktop_tracking_robot
./scripts/rpi_start_ros.sh
```

这会启动：

- `robot_bringup/rpi_stack.launch.py`
- `uart_bridge_node`

## 4. WSL2 启动视觉主链

```bash
ros2 launch robot_bringup vision_stack.launch.py
```

默认会启动：

- `gst_receiver_node`
- `stereo_splitter_node`
- `detection_node`
- `tracker_node`
- `behavior_node`
- `leg_motion_node`

## 5. 无硬件模式

只想验证 ROS 2 主链和控制闭环时：

```bash
source /opt/ros/jazzy/setup.bash
cd /home/peter/dog/dog_dev
source ros2_ws/install/setup.bash
ros2 launch robot_bringup test_73_complete.launch.py
```

这个入口会在 WSL2 上额外启动 `mock_uart_bridge_node`。

## 6. 常用观察命令

```bash
ros2 node list
ros2 topic list
ros2 topic hz /stereo/image_raw
ros2 topic hz /tracked_objects
ros2 topic echo /pixel_error
ros2 topic echo /servo_state
ros2 service call /set_target_class robot_interfaces/srv/SetTargetClass "{class_name: 'cup'}"
```

## 7. 判断系统是否真正跑通

- WSL2 能看到 `/stereo/image_raw`
- `detection_node` 持续发布 `/detections`
- `tracker_node` 持续发布带 `track_id` 的 `/tracked_objects`
- `behavior_node` 在检测到目标类别时持续发布 `/pixel_error`
- Pi 上 `uart_bridge_node` 不再打印握手未完成告警
- `/servo_state` 持续更新
