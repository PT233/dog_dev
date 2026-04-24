# 🚀 快速开始指南

**适用于**：硬件接线完成后快速启动系统

---

## 环境变量（两端都需要）

```bash
# 添加到 ~/.bashrc
echo 'export ROS_DOMAIN_ID=42' >> ~/.bashrc
echo 'export ROS_LOCALHOST_ONLY=0' >> ~/.bashrc
source ~/.bashrc
```

---

## 启动流程（4个终端，按顺序）

### Terminal 1：树莓派 - 启动相机推流

```bash
ssh ubuntu@192.168.137.100
cd ~/desktop_tracking_robot
./scripts/start_camera_stream.sh

# 预期输出：
# [GStreamer] Pipeline running...
# [V4L2] Reading from /dev/video0
```

### Terminal 2：树莓派 - 启动 uart_bridge

```bash
ssh ubuntu@192.168.137.100
source ~/ros2_ws/install/setup.bash
export ROS_DOMAIN_ID=42
ros2 launch robot_bringup rpi_stack.launch.py

# 预期输出：
# [INFO] uart_bridge_node started
# [INFO] Subscribed to /servo_cmd
```

### Terminal 3：WSL2 - 启动视觉管道

```bash
cd ~/dog/dog_dev
source install/setup.bash
export ROS_DOMAIN_ID=42
ros2 launch robot_bringup vision_stack.launch.py

# 预期输出：
# [INFO] gst_receiver_node: publishing to /stereo/image_raw
# [INFO] detection_node: model loaded
# [INFO] tracker_node: initialized
# [INFO] behavior_node: started
# [INFO] visual_servo_node: started
```

### Terminal 4：WSL2 - 验证系统（可选）

```bash
# 监控实时话题
ros2 topic echo /tracked_objects
ros2 topic echo /pixel_error
ros2 topic echo /servo_state

# 切换目标类别
ros2 service call /set_target_class robot_interfaces/srv/SetTargetClass "{class_name: 'cup'}"

# 实时显示误差曲线
rqt_plot /pixel_error/x /pixel_error/y &
```

---

## 常见命令速查

### 话题监控
```bash
# 列出所有话题
ros2 topic list

# 显示话题频率
ros2 topic hz /tracked_objects

# 查看话题消息内容
ros2 topic echo /servo_cmd
```

### 参数调整
```bash
# 查看当前参数
ros2 param list /visual_servo_node

# 动态修改参数（无需重启）
ros2 param set /visual_servo_node yaw.kp 0.06
ros2 param set /visual_servo_node pitch.ki 0.002
```

### 调试工具
```bash
# 启动图形化接口
rqt

# 启动图像查看器
rqt_image_view

# 启动参数动态调整面板
rqt_reconfigure

# 启动 ROS 2 实时绘图
rqt_plot /pixel_error/x /pixel_error/y
```

### 问题诊断
```bash
# 检查 ROS 2 节点
ros2 node list

# 查看节点详细信息
ros2 node info /behavior_node

# 查看 UART 通信（树莓派）
sudo cat /dev/ttyAMA0

# 检查网络连接
ping 192.168.137.100
```

---

## 性能指标

| 指标 | 目标 | 验证命令 |
|---|---|---|
| 图像推流 | ~30 FPS | `ros2 topic hz /stereo/image_raw` |
| 目标检测 | ~30 Hz | `ros2 topic hz /detections` |
| 目标跟踪 | ~30 Hz | `ros2 topic hz /tracked_objects` |
| 舵机控制 | 30 Hz | `ros2 topic hz /servo_cmd` |
| 像素误差 | <5 px | `ros2 topic echo /pixel_error` |

---

## 故障排查

### 问题 1：舵机不动

```bash
# 检查 1：是否有 /servo_cmd 话题
ros2 topic list | grep servo_cmd

# 检查 2：是否有 UART 通信
ros2 topic echo /servo_state

# 检查 3：UART 连接
ssh ubuntu@192.168.137.100 'ls -l /dev/ttyAMA0'

# 解决方案：重启 uart_bridge
# Terminal 2 中 Ctrl+C，然后重新启动
```

### 问题 2：网络不同步

```bash
# 确认 ROS_DOMAIN_ID
echo $ROS_DOMAIN_ID

# 在两端都应输出：42
# 如果不同，重新设置：
export ROS_DOMAIN_ID=42
source /opt/ros/jazzy/setup.bash

# 清除 ROS 缓存
pkill rmw_fastrtps_cpp
sleep 2
ros2 topic list
```

### 问题 3：检测框不出现

```bash
# 检查检测节点是否运行
ros2 node list | grep detection

# 检查是否有输入图像
ros2 topic echo /camera/image_mono --limit 1

# 检查检测输出
ros2 topic echo /detections --limit 5
```

---

## PID 快速调参

### 症状 → 调整映射

| 症状 | 调整 |
|---|---|
| 反应迟钝，跟踪滞后 | `Kp` ↑ (0.05 → 0.07) |
| 振荡、超冲过大 | `Kp` ↓ 或 `Kd` ↑ |
| 稳态有偏差 | `Ki` ↑ (0.001 → 0.002) |
| 动作抖动 | `Kd` ↑ 或 `Ki` ↓ |

### 动态调参示例

```bash
# Terminal 4 中实时绘制误差曲线
rqt_plot /pixel_error/x /pixel_error/y

# 另一个 Terminal 动态调整参数
ros2 param set /visual_servo_node yaw.kp 0.06
ros2 param set /visual_servo_node yaw.kd 0.03

# 观察曲线变化，找到最优参数
```

---

## 关闭系统

**顺序很重要（反向启动顺序）**

```bash
# Terminal 3: Ctrl+C 停止 WSL2 节点
# Terminal 2: Ctrl+C 停止树莓派节点
# Terminal 1: Ctrl+C 停止相机推流
# Terminal 4: Ctrl+C 停止监控

# 验证所有进程已停止
pkill -f "ros2 launch"
pkill -f "gst-launch"
```

---

## 下一步

- **调参**：见 `docs/pid_tuning.md`
- **硬件**：见 `hardware_wiring.html`
- **完整指南**：见 `SETUP_GUIDE.md`
- **架构设计**：见 `architecture.md`

---

**Last Updated**: 2026-04-24
