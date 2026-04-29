# 架构文档

基线日期：`2026-04-28`

本文档只描述当前代码实际实现的结构，不描述已经删除的旧节点名、旧目录名或历史方案。

## 1. 系统概览

系统由三个运行域组成：

1. `WSL2 / PC`: 视频接收、图像裁切、目标检测、目标跟踪、目标选择、视觉伺服
2. `Raspberry Pi`: USB 双目摄像头推流、UART 与 ROS 2 桥接
3. `STM32F103`: 舵机轨迹规划、PWM 输出、状态回传、握手状态机

主链路如下：

```text
/dev/video0
  -> H.264/UDP:5600
  -> /stereo/image_raw
  -> ~/input/image
  -> /detection_node/output/detections
  -> /tracker_node/output/tracked_objects
  -> /behavior_node/output/pixel_error
  -> /leg_motion_node/output/servo_command
  -> UART
  -> SG90 x4
  -> /uart_bridge_node/output/servo_state
```

## 2. ROS 2 包与可执行文件

| 包 | 可执行文件 / 入口 | 默认部署位置 | 职责 |
| --- | --- | --- | --- |
| `gst_receiver` | `gst_receiver_node` | WSL2 | 监听 `5600/UDP`，把 H.264 视频解码成 `/stereo/image_raw` |
| `stereo_splitter` | `stereo_splitter_node` | WSL2 | 裁出左目图像，发布 `~/input/image` |
| `detection_node` | `detection_node_exe` | WSL2 | YOLOv8 ONNX 推理，发布 `/detection_node/output/detections` |
| `tracker_node` | `tracker_node_exe` | WSL2 | ByteTrack 风格的轨迹维护，发布 `/tracker_node/output/tracked_objects` |
| `behavior_node` | `behavior_node_exe` | WSL2 | 选择目标类别，计算像素误差，提供 `/behavior_node/input/set_target_class` |
| `visual_servo` | `visual_servo_node_exe` | WSL2 | `leg_motion_node`，把误差转换为四足舵机目标角 |
| `uart_bridge` | `uart_bridge_node` | Raspberry Pi | `/leg_motion_node/output/servo_command <-> UART <-> /uart_bridge_node/output/servo_state` |
| `robot_bringup` | launch 文件集合 | WSL2 / Raspberry Pi | 组合启动入口 |
| `robot_interfaces` | msg/srv 定义 | 两端 | 共享消息和服务接口 |

附加项：

- `detection_viz_node_exe`: 调试用可视化节点
- `vision_front`: 可选的进程内组合入口，默认不构建
- `mock_uart_bridge`: 由 `scripts/mock_uart_bridge.py` 安装到 `robot_bringup`，供无硬件集成测试使用

## 3. Launch 入口

| 文件 | 运行位置 | 内容 |
| --- | --- | --- |
| `ros2_ws/src/robot_bringup/launch/vision_stack.launch.py` | WSL2 | `gst_receiver + stereo_splitter + detection + tracker + behavior + visual_servo` |
| `ros2_ws/src/robot_bringup/launch/rpi_stack.launch.py` | Raspberry Pi | `uart_bridge` |
| `ros2_ws/src/robot_bringup/launch/test_73_complete.launch.py` | WSL2 | 视觉栈 + `mock_uart_bridge` |
| `ros2_ws/src/robot_bringup/launch/vision_front.launch.py` | WSL2 | 单进程实验入口 |

## 4. Topics 与 Services

### Topics

| Topic | 类型 | 发布者 | 订阅者 |
| --- | --- | --- | --- |
| `/stereo/image_raw` | `sensor_msgs/Image` | `gst_receiver_node` | `stereo_splitter_node` |
| `~/input/image` | `sensor_msgs/Image` | `stereo_splitter_node` | `detection_node` |
| `/detection_node/output/detections` | `robot_interfaces/msg/Detection2DArray` | `detection_node` | `tracker_node` |
| `/tracker_node/output/tracked_objects` | `robot_interfaces/msg/Detection2DArray` | `tracker_node` | `behavior_node` |
| `/behavior_node/output/pixel_error` | `geometry_msgs/Vector3` | `behavior_node` | `leg_motion_node` |
| `/leg_motion_node/output/servo_command` | `sensor_msgs/JointState` | `leg_motion_node` | `uart_bridge_node` |
| `/uart_bridge_node/output/servo_state` | `sensor_msgs/JointState` | `uart_bridge_node` | `leg_motion_node` |

### Services

| Service | 类型 | 服务端 | 状态 |
| --- | --- | --- | --- |
| `/behavior_node/input/set_target_class` | `robot_interfaces/srv/SetTargetClass` | `behavior_node` | 已实现 |
| `/calibrate_center` | `robot_interfaces/srv/CalibrateCenter` | 无 | 仅保留接口定义 |

## 5. 节点内部职责摘要

### `detection_node`

- 订阅 `~/input/image`
- 使用独立推理线程处理图像队列
- 默认参数：`model_path`、`conf_threshold`、`nms_threshold`、`use_cuda`
- 输出 `Detection2DArray`

### `tracker_node`

- 输入 `/detection_node/output/detections`
- 根据中心点和 IoU 风格匹配更新轨迹
- 输出带 `track_id` 的 `/tracker_node/output/tracked_objects`

### `behavior_node`

- 从 `/tracker_node/output/tracked_objects` 中筛出指定 `class_id`
- 当前策略：同类别目标中选择面积最大的框
- 发布像素误差：`(target_center_x - center_x, target_center_y - center_y, 0)`
- 支持 `/behavior_node/input/set_target_class`

### `leg_motion_node`

- 订阅 `/behavior_node/output/pixel_error` 与 `/uart_bridge_node/output/servo_state`
- 内部含两组 PID：`turn.*` 和 `forward.*`
- 将转向偏置和步幅叠加到四条腿的中立角上
- 30 Hz 定时发布 `/leg_motion_node/output/servo_command`

### `uart_bridge_node`

- 打开 `/dev/ttyAMA0`
- 在握手未完成前丢弃 `/leg_motion_node/output/servo_command`
- 接收 `SERVO_STATE_V2` 后映射时间戳、检测丢包、发布 `/uart_bridge_node/output/servo_state`

## 6. UART 协议摘要

共享协议定义位于 `shared/uart_protocol.h`，当前版本号是 `3`。

### 帧格式

```text
AA 55 | cmd_id | payload_len | payload... | crc16(lo,hi) | 0D
```

### 关键命令

| 命令 | 值 | 方向 | 含义 |
| --- | --- | --- | --- |
| `UART_CMD_SERVO_CONTROL` | `0x01` | Pi -> STM32 | 舵机角目标 |
| `UART_CMD_INIT_HANDSHAKE` | `0x10` | Pi -> STM32 | 请求进入 `ACTIVE` |
| `UART_CMD_SERVO_STATE_V2` | `0x82` | STM32 -> Pi | 带 `timestamp_ms` 和 `frame_seq` 的状态回报 |
| `UART_CMD_SYSTEM_STATE` | `0x83` | STM32 -> Pi | `BOOT_CENTERING / WAITING_CONNECTION / ACTIVE / ERROR` |

### 握手状态机

1. STM32 上电进入 `BOOT_CENTERING`
2. `500 ms` 后进入 `WAITING_CONNECTION`
3. `uart_bridge` 周期发送 `INIT_HANDSHAKE`
4. STM32 收到正确的握手负载后进入 `ACTIVE`
5. `uart_bridge` 只有在收到 `ACTIVE` 后才转发 `/leg_motion_node/output/servo_command`

## 7. 参数文件

| 文件 | 实际消费者 | 说明 |
| --- | --- | --- |
| `config/behavior.yaml` | `behavior_node` | 图像尺寸与画面中心 |
| `config/visual_servo.yaml` | `leg_motion_node` | 控制周期、角度限幅、PID 参数 |
| `ros2_ws/src/detection_node/config/detection.yaml` | `detection_node` | 模型路径、阈值、ONNX Runtime 参数 |
| `ros2_ws/src/tracker_node/config/tracker.yaml` | `tracker_node` | 跟踪器阈值 |
| `ros2_ws/src/uart_bridge/config/uart_bridge.yaml` | `uart_bridge_node` | UART 设备路径与波特率 |

## 8. 构建与部署边界

### WSL2

- 在 `ros2_ws/` 下运行 `colcon build`
- `detection_node` 需要 `ONNXRUNTIME_ROOT`
- 启动前需要 `source scripts/ros2_network_env.sh`

### Raspberry Pi

- 只需要 `robot_interfaces`、`uart_bridge`、`robot_bringup`
- 推荐使用 `scripts/rpi_one_click_deploy.sh`
- 可选使用 `scripts/rpi_cross_build.sh` 生成 ARM64 tarball 后部署

### STM32

- 工程位于 `stm32_keil/STM32.uvprojx`
- 共享协议镜像位于 `stm32_keil/Core/Inc/uart_protocol.h`

## 9. 当前已知缺口

- `/calibrate_center` 尚未实现服务端
- 没有覆盖整条主链的自动化端到端测试
- `vision_front` 仍是实验入口，不是默认部署方式

## 10. 相关文档

- [README.md](README.md)
- [QUICK_START.md](QUICK_START.md)
- [SETUP_GUIDE.md](SETUP_GUIDE.md)
- [docs/README.md](docs/README.md)
