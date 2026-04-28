# 验证清单

## 1. 文档同步后的最低验证

### Launch 文件语法

```bash
python3 -m py_compile ros2_ws/src/robot_bringup/launch/vision_stack.launch.py
python3 -m py_compile ros2_ws/src/robot_bringup/launch/rpi_stack.launch.py
python3 -m py_compile ros2_ws/src/robot_bringup/launch/test_73_complete.launch.py
```

### 关键路径存在

```bash
rg --files ros2_ws/src/robot_bringup/launch
rg --files ros2_ws/src/uart_bridge/config
rg --files config
```

## 2. 构建验证

### WSL2

```bash
source /opt/ros/jazzy/setup.bash
export ONNXRUNTIME_ROOT=/path/to/onnxruntime
cd ros2_ws
colcon build
```

### Raspberry Pi

```bash
source /opt/ros/jazzy/setup.bash
cd ~/ros2_ws
colcon build --merge-install \
  --packages-select robot_interfaces uart_bridge robot_bringup
```

## 3. 运行时验证

### 视觉链

```bash
ros2 topic hz /stereo/image_raw
ros2 topic hz /detections
ros2 topic hz /tracked_objects
ros2 topic echo /pixel_error
```

### Pi 串口桥

```bash
ros2 topic echo /servo_state
ros2 topic echo /servo_cmd
```

### 服务

```bash
ros2 service call /set_target_class robot_interfaces/srv/SetTargetClass "{class_name: 'cup'}"
```

## 4. STM32 / UART 验证

- STM32 上电后先进入 `BOOT_CENTERING`
- `uart_bridge_node` 持续发送握手
- STM32 进入 `ACTIVE` 后，`/servo_cmd` 不再被丢弃
- `SERVO_STATE_V2` 持续回到 Pi，并映射成 `/servo_state`

## 5. 当前没有自动化覆盖的点

- `/calibrate_center` 功能
- 全链路硬件端到端回归
- 舵机机械零位校准
