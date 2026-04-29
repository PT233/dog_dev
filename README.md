# desktop_tracking_robot

基于 ROS 2 Jazzy、GStreamer、YOLOv8、ByteTrack、Raspberry Pi 和 STM32F103 的桌面目标跟随机器人。

本文档集已按当前重构后的代码树重新整理，基线日期为 `2026-04-28`。默认运行链路是：

`gst_receiver -> stereo_splitter -> detection_node -> tracker_node -> behavior_node -> leg_motion_node -> uart_bridge -> STM32`

## 从哪里开始

- [QUICK_START.md](QUICK_START.md): 已完成环境准备后的最短启动路径
- [SETUP_GUIDE.md](SETUP_GUIDE.md): 按主机划分的构建、部署、接线和环境准备
- [ARCHITECTURE.md](ARCHITECTURE.md): 当前代码结构、运行时拓扑、消息接口和协议摘要
- [docs/README.md](docs/README.md): 专题文档索引
- [hardware_wiring.html](hardware_wiring.html): 接线图

## 当前代码主链

### WSL2 / PC

- `gst_receiver_node`: 接收树莓派发来的 `UDP/H.264` 视频并发布 `/stereo/image_raw`
- `stereo_splitter_node`: 从 `640x480` 双目拼接图中裁出左目，发布 `~/input/image`
- `detection_node`: 运行 YOLOv8 ONNX 推理，发布 `/detection_node/output/detections`
- `tracker_node`: 给检测结果分配稳定轨迹 ID，发布 `/tracker_node/output/tracked_objects`
- `behavior_node`: 按目标类别筛选目标并发布 `/behavior_node/output/pixel_error`
- `leg_motion_node`: 把像素误差转换为四足舵机角命令，发布 `/leg_motion_node/output/servo_command`

### Raspberry Pi

- `scripts/start_camera_stream.sh`: `/dev/video0 -> H.264 -> UDP:5600`
- `uart_bridge_node`: `/leg_motion_node/output/servo_command -> UART`，`UART -> /uart_bridge_node/output/servo_state`

### STM32

- `Task_UART_RX`: DMA + IDLE 收帧、握手、分发舵机命令
- `Task_Traj_Planner`: 5 ms 周期梯形轨迹插值
- `Task_Status_TX`: 50 ms 周期上报 `SERVO_STATE_V2` 和 `SYSTEM_STATE`
- `Task_Safety`: IWDG 喂狗

## 仓库结构

```text
dog_dev/
├── README.md
├── QUICK_START.md
├── SETUP_GUIDE.md
├── ARCHITECTURE.md
├── hardware_wiring.html
├── config/                    # 顶层共享参数文件
├── docs/                      # 专题文档
├── models/                    # YOLO 模型与 COCO 标签
├── ros2_ws/
│   └── src/
│       ├── robot_bringup
│       ├── robot_interfaces
│       ├── gst_receiver
│       ├── stereo_splitter
│       ├── detection_node
│       ├── tracker_node
│       ├── behavior_node
│       ├── visual_servo
│       └── uart_bridge
├── scripts/                   # 启动、部署和网络配置脚本
├── shared/                    # ROS 侧与 STM32 侧共享头文件/小工具
├── stm32_keil/                # STM32F103 工程
└── tests/                     # 离线测试与验证脚本
```

## 最常用命令

### WSL2 构建

```bash
source /opt/ros/jazzy/setup.bash
export ONNXRUNTIME_ROOT=/path/to/onnxruntime
cd /home/peter/dog/dog_dev/ros2_ws
colcon build
source install/setup.bash
```

### WSL2 启动视觉栈

```bash
source /opt/ros/jazzy/setup.bash
cd /home/peter/dog/dog_dev
source ros2_ws/install/setup.bash
export ROBOT_DDS_ROLE=wsl
source scripts/ros2_network_env.sh
ros2 launch robot_bringup vision_stack.launch.py
```

### 树莓派启动

```bash
cd /home/ubuntu/desktop_tracking_robot
./scripts/rpi_start_camera.sh <WSL_IP> 5600
./scripts/rpi_start_ros.sh
```

### 无硬件联调

```bash
source /opt/ros/jazzy/setup.bash
cd /home/peter/dog/dog_dev
source ros2_ws/install/setup.bash
ros2 launch robot_bringup test_73_complete.launch.py
```

## 当前约束

- `detection_node` 构建依赖 `ONNXRUNTIME_ROOT`
- `vision_front` 是可选的进程内组合可执行文件，默认 `BUILD_VISION_FRONT=OFF`
- `detection_viz_node` 仅用于调试，不在默认 launch 链路中
- `robot_interfaces/srv/CalibrateCenter.srv` 目前只有接口定义，代码里还没有服务端实现

## 文档维护规则

- 只要 launch、可执行文件名、topic/service、共享协议或部署脚本有变化，就同步更新顶层文档和 `docs/`
- `progress.md` 保存当前状态快照，不再维护旧式逐任务流水账
- `task.md` 保存当前待办，不再对应旧的阶段式生成任务模板
