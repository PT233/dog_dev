# 物体识别与跟随机器人 · 架构设计文档

**项目名称**：`desktop_tracking_robot`
**架构版本**：v3.0
**目标场景**：桌面玩具机器人基于双目摄像头识别 COCO 80 类物体并用舵机持续跟随

---

## 目录

1. [硬件与软件总览](#1-硬件与软件总览)
2. [仓库与文件夹结构](#2-仓库与文件夹结构)
3. [各组件职责](#3-各组件职责)
4. [状态存储位置](#4-状态存储位置)
5. [服务之间的连接](#5-服务之间的连接)
6. [消息与接口定义](#6-消息与接口定义)
7. [部署与启动顺序](#7-部署与启动顺序)
8. [开发阶段划分](#8-开发阶段划分)

---

## 1. 硬件与软件总览

### 1.1 硬件清单

| 设备 | 规格 | 角色 |
|---|---|---|
| PC (WSL2) | Win11 + Ubuntu 24.04 + NVIDIA GPU (CUDA) | 视觉处理、AI 推理、决策 |
| 树莓派 4B | 2GB + 4GB swap，Ubuntu Server 24.04 | 相机接入、协议桥接 |
| STM32F103C8T6 | 64KB Flash / 20KB RAM | 实时舵机控制 |
| USB 双目摄像头 | 640×480（左右各 320×480 拼接） | 视觉输入 |
| SG90 × 4 | 独立 5V 电源 | 执行机构（Yaw + Pitch + 预留 2 路） |
| MPU6050 | 接树莓派 I2C（阶段 2 启用） | IMU 传感器 |

### 1.2 网络拓扑

```
┌──────────────┐    网线直连     ┌────────────┐     GPIO UART      ┌──────────┐
│   WSL2 (PC)  │ ─────────────── │  树莓派 4B  │ ───────────────── │  STM32   │
│ 192.168.10.1 │  UDP + ROS 2    │192.168.10.2│  921600 bps + CRC  │          │
└──────────────┘                 └────────────┘                     └──────────┘
```

### 1.3 通信协议矩阵

| 链路 | 协议 | 内容 | 特点 |
|---|---|---|---|
| PC ↔ 树莓派（图像） | GStreamer UDP:5600 | H.264 视频流 | 高带宽、低延迟、非 ROS |
| PC ↔ 树莓派（控制） | ROS 2 DDS（mirrored） | 话题/服务/Action | 结构化、可调试 |
| 树莓派 ↔ STM32 | UART 二进制帧 | 舵机命令/状态反馈 | 自定义协议 + CRC16 |

---

## 2. 仓库与文件夹结构

### 2.1 顶层结构

```
desktop_tracking_robot/
├── README.md
├── architecture.md                  # 本文档
├── docs/                            # 额外文档（PID 调参、协议规范等）
│   ├── uart_protocol.md
│   ├── pid_tuning.md
│   └── gstreamer_pipeline.md
│
├── ros2_ws/                         # ROS 2 工作空间（WSL2 + 树莓派共用源码）
│   └── src/
│       ├── robot_interfaces/        # 自定义消息/服务
│       ├── robot_bringup/           # launch 文件 + 参数配置
│       │
│       ├── gst_receiver/            # [WSL2] GStreamer → ROS 2 桥接
│       ├── stereo_splitter/         # [WSL2] 双目拼接图分割
│       ├── detection_node/          # [WSL2] YOLOv8 + ONNX Runtime
│       ├── tracker_node/            # [WSL2] ByteTrack 跟踪器
│       ├── behavior_node/           # [WSL2] 目标选择 + 像素误差
│       ├── visual_servo/            # [WSL2] PID 视觉伺服
│       │
│       ├── camera_streamer/         # [树莓派] GStreamer 推流启动器
│       ├── uart_bridge/             # [树莓派] UART ↔ ROS 2 桥接
│       └── imu_node/                # [树莓派] MPU6050 驱动（阶段 2）
│
├── stm32_fw/                        # STM32 固件（CubeIDE 工程）
│   ├── Core/
│   │   ├── Inc/
│   │   │   ├── uart_protocol.h      # 与树莓派共用的协议定义
│   │   │   ├── traj_planner.h
│   │   │   └── servo_driver.h
│   │   └── Src/
│   │       ├── main.c
│   │       ├── uart_protocol.c
│   │       ├── traj_planner.c       # 梯形速度插值
│   │       └── servo_driver.c       # PWM 输出
│   ├── Middlewares/FreeRTOS/
│   └── Makefile
│
├── shared/                          # 跨平台共享代码（C 头文件）
│   └── uart_protocol.h              # 协议帧定义，树莓派和 STM32 共用
│
├── models/                          # AI 模型文件（.gitignore 中用 LFS）
│   ├── yolov8n.onnx
│   └── coco_classes.txt
│
├── config/                          # 运行时配置（YAML）
│   ├── detection.yaml
│   ├── tracker.yaml
│   ├── behavior.yaml
│   ├── visual_servo.yaml
│   └── uart_bridge.yaml
│
├── scripts/                         # 辅助脚本
│   ├── setup_wsl2.sh                # WSL2 环境一键配置
│   ├── setup_rpi.sh                 # 树莓派一键配置
│   ├── export_yolo_onnx.py          # YOLOv8 模型导出
│   ├── start_camera_stream.sh       # 树莓派端 GStreamer 推流
│   └── calibrate_camera.py          # 相机标定
│
└── tests/                           # 离线测试
    ├── test_uart_protocol/
    ├── test_detection/
    └── test_video.mp4               # 离线视频用于回归测试
```

### 2.2 ROS 2 包内部结构（以 `detection_node` 为例）

```
detection_node/
├── CMakeLists.txt
├── package.xml
├── include/detection_node/
│   ├── detection_node.hpp           # ROS 2 节点类
│   ├── yolo_infer.hpp               # ONNX Runtime 推理封装
│   └── postprocess.hpp              # NMS + letterbox 逆变换
├── src/
│   ├── detection_node.cpp
│   ├── yolo_infer.cpp
│   ├── postprocess.cpp
│   └── main.cpp
├── config/
│   └── detection.yaml               # 默认参数
├── launch/
│   └── detection.launch.py
└── test/
    └── test_postprocess.cpp
```

---

## 3. 各组件职责

### 3.1 WSL2 端节点

#### `gst_receiver_node`

| 项 | 说明 |
|---|---|
| **职责** | 从 UDP:5600 接收 H.264 流，解码为 BGR 图像，发布 ROS 2 话题 |
| **输入** | GStreamer UDP 流（外部，非 ROS） |
| **输出** | `/stereo/image_raw` (sensor_msgs/Image, BGR8) |
| **实现** | C++ + GStreamer appsink + OpenCV |
| **关键点** | 时间戳从 RTP PTS 回溯，不用 `this->now()` |

#### `stereo_splitter_node`

| 项 | 说明 |
|---|---|
| **职责** | 将 640×480 双目拼接图切成左右两张 320×480 |
| **输入** | `/stereo/image_raw` |
| **输出** | `/camera/image_mono` (只发左半，用于检测) + `/camera/right/image` (预留) |
| **实现** | C++ + `cv::Mat(src, cv::Rect(0,0,320,480))` |

#### `detection_node` ⭐

| 项 | 说明 |
|---|---|
| **职责** | 跑 YOLOv8n 目标检测 |
| **输入** | `/camera/image_mono` |
| **输出** | `/detections` (vision_msgs/Detection2DArray) |
| **实现** | C++ + ONNX Runtime 1.17 + CUDAExecutionProvider |
| **模型** | `models/yolov8n.onnx` (COCO 80 类, 6MB) |
| **性能** | ~15ms/帧推理（GPU），30 FPS |
| **关键点** | 启动时 warm-up 一次（传全黑图）避免首帧卡顿 |

#### `tracker_node` ⭐

| 项 | 说明 |
|---|---|
| **职责** | ByteTrack 帧间关联，给每个检测框分配持久 track_id |
| **输入** | `/detections` |
| **输出** | `/tracked_objects` (vision_msgs/Detection2DArray，带 id 字段) |
| **实现** | C++ + ByteTrack 开源库 + Eigen |
| **状态** | 内存中维护活跃 track 列表（卡尔曼滤波状态） |
| **关键点** | 遮挡 30 帧内 ID 保持稳定 |

#### `behavior_node`

| 项 | 说明 |
|---|---|
| **职责** | 根据目标类别从所有跟踪对象中选一个作为当前目标，计算像素误差 |
| **输入** | `/tracked_objects` + `/set_target_class` (service) |
| **输出** | `/pixel_error` (geometry_msgs/Vector3) |
| **实现** | C++ 状态机 |
| **状态** | 当前目标类别（从 service 设置）、当前锁定的 track_id |
| **关键点** | 多目标时选最大 bbox 或最高 confidence |

#### `visual_servo_node` ⭐

| 项 | 说明 |
|---|---|
| **职责** | PID 控制器，像素误差 → 舵机目标角度 |
| **输入** | `/pixel_error` + `/servo_state` (当前角度反馈) |
| **输出** | `/servo_cmd` (sensor_msgs/JointState) |
| **频率** | 30Hz 控制环 |
| **参数** | Kp/Ki/Kd、死区、输出限幅，从 `config/visual_servo.yaml` 加载 |
| **关键点** | 输出是**绝对角度**，不是增量；输入限幅防止超出机械极限 |

### 3.2 树莓派端节点

#### `gst_pipeline`（外部进程，非 ROS）

| 项 | 说明 |
|---|---|
| **职责** | 读取 `/dev/video0` 双目 USB，硬件 H.264 编码，UDP 推流到 WSL2 |
| **实现** | `gst-launch-1.0` 命令行 + systemd 服务 |
| **管道** | `v4l2src → jpegdec → v4l2h264enc → rtph264pay → udpsink` |
| **启动** | `scripts/start_camera_stream.sh` |

#### `uart_bridge_node` ⭐

| 项 | 说明 |
|---|---|
| **职责** | ROS 2 话题 ↔ UART 二进制帧双向转换 |
| **输入** | `/servo_cmd` (sensor_msgs/JointState) |
| **输出** | `/servo_state` (sensor_msgs/JointState) |
| **串口** | `/dev/ttyAMA0` @ 921600 bps |
| **实现** | C++ + Boost.Asio 或 POSIX termios |
| **状态** | 最近发送的指令序号（用于 ack 匹配） |
| **关键点** | 订阅/servo_cmd 使用 `rclcpp::SensorDataQoS()`，避免堆积 |

#### `imu_node`（阶段 2）

| 项 | 说明 |
|---|---|
| **职责** | 从 I2C 读 MPU6050，发布 IMU 话题 |
| **输入** | I2C bus（GPIO 2/3） |
| **输出** | `/imu/data` (sensor_msgs/Imu) |
| **频率** | 100 Hz |

### 3.3 STM32 固件任务

| 任务 | 职责 | 周期/触发 |
|---|---|---|
| `UART_RX_Task` | DMA + IDLE 中断接收，CRC 校验，入队解析 | 中断驱动 |
| `Traj_Planner_Task` | 梯形速度曲线：目标角度 → 5ms 粒度中间角度 | 5ms 周期 |
| `PWM_Output_Task` | 根据当前角度设置 TIM2/TIM3 CCR 值 | Traj 触发 |
| `Status_TX_Task` | 周期上报所有舵机当前角度给树莓派 | 50ms 周期（20Hz） |
| `Safety_Task` | 500ms 无新指令 → 保持位置；IWDG 喂狗 | 100ms 周期 |

---

## 4. 状态存储位置

本系统的状态存储遵循**按生命周期分层**的原则：

### 4.1 编译时常量

| 内容 | 位置 | 说明 |
|---|---|---|
| 协议帧头/CMD_ID | `shared/uart_protocol.h` | 树莓派和 STM32 共用 |
| COCO 类别表 | `models/coco_classes.txt` | 启动时加载到 detection_node |
| 舵机 ID 定义 | `shared/uart_protocol.h` | 0=Yaw, 1=Pitch, 2/3=预留 |

### 4.2 运行时配置（YAML，启动时读取）

| 文件 | 内容 | 使用节点 |
|---|---|---|
| `config/detection.yaml` | 模型路径、置信度阈值、NMS 阈值、CUDA 设备号 | detection_node |
| `config/tracker.yaml` | ByteTrack 参数（track_thresh、match_thresh、track_buffer） | tracker_node |
| `config/behavior.yaml` | 默认目标类别、多目标选择策略 | behavior_node |
| `config/visual_servo.yaml` | Kp/Ki/Kd、死区、输出限幅、控制频率 | visual_servo_node |
| `config/uart_bridge.yaml` | 串口设备、波特率、超时 | uart_bridge_node |

所有 YAML 通过 launch 文件的 `parameters=[...]` 加载，运行时可用 `ros2 param set` 动态修改。

### 4.3 运行时状态（内存中，不持久化）

| 状态 | 归属节点 | 访问方式 |
|---|---|---|
| 活跃 track 列表（卡尔曼状态） | tracker_node | 节点内部 |
| 当前目标类别 | behavior_node | `/set_target_class` service 修改 |
| 当前锁定的 track_id | behavior_node | 节点内部，目标切换时重置 |
| PID 积分项、上次误差 | visual_servo_node | 节点内部 |
| 舵机当前角度（缓存） | uart_bridge_node | 从 STM32 反馈更新 |
| 最近发送指令序号 | uart_bridge_node | 节点内部 |
| 4 路舵机目标/当前角度 | STM32（全局变量） | 受互斥锁保护 |

**关键原则**：所有运行时状态都在节点内存，**节点重启即重置**。没有数据库、没有文件持久化。

### 4.4 日志与调试数据

| 类型 | 位置 | 清理策略 |
|---|---|---|
| ROS 2 日志 | `~/.ros/log/` | 每个节点启动自动分目录 |
| rosbag 录制 | `~/bags/` | 手动管理 |
| rqt_plot 数据 | 内存 | 工具关闭即丢 |

### 4.5 模型文件

| 文件 | 路径 | 加载时机 |
|---|---|---|
| yolov8n.onnx | `models/yolov8n.onnx` | detection_node 启动时加载到 GPU |
| coco_classes.txt | `models/coco_classes.txt` | detection_node 启动时读入内存 |

---

## 5. 服务之间的连接

### 5.1 数据流总览（正向：视觉 → 动作）

```
┌──────────────┐
│ USB 双目相机 │ /dev/video0
└──────┬───────┘
       │ YUYV 640×480 @ 30fps
       ▼
┌──────────────┐
│ gst_pipeline │ [树莓派，非 ROS]
│ (V4L2+H264)  │
└──────┬───────┘
       │ H.264 RTP over UDP:5600
       ▼（跨主机）
┌──────────────┐
│gst_receiver  │ [WSL2]
│   _node      │
└──────┬───────┘
       │ /stereo/image_raw (sensor_msgs/Image)
       ▼
┌──────────────┐
│  stereo_     │ [WSL2]
│  splitter    │
└──────┬───────┘
       │ /camera/image_mono
       ▼
┌──────────────┐
│ detection_   │ [WSL2 + CUDA]
│   node       │  ONNX Runtime + YOLOv8n
└──────┬───────┘
       │ /detections (vision_msgs/Detection2DArray)
       ▼
┌──────────────┐
│  tracker_    │ [WSL2]
│   node       │  ByteTrack
└──────┬───────┘
       │ /tracked_objects
       ▼
┌──────────────┐      /set_target_class (service)
│ behavior_    │ ◄──────────────────────────── 人工/语音
│   node       │
└──────┬───────┘
       │ /pixel_error (geometry_msgs/Vector3)
       ▼
┌──────────────┐      /servo_state (反馈回环)
│ visual_servo │ ◄──────────────────┐
│   _node      │                     │
└──────┬───────┘                     │
       │ /servo_cmd (JointState)     │
       ▼（跨主机，DDS）              │
┌──────────────┐                     │
│ uart_bridge_ │ [树莓派]            │
│   node       │ ──►/servo_state──┐  │
└──────┬───────┘                  │  │
       │ UART 二进制帧            │  │
       ▼                          │  │
┌──────────────┐                  │  │
│   STM32      │                  │  │
│  FreeRTOS    │                  │  │
└──────┬───────┘                  │  │
       │ PWM 50Hz                 │  │
       ▼                          │  │
┌──────────────┐                  │  │
│  SG90 × 4    │                  │  │
└──────────────┘                  │  │
                                  │  │
                 (跨主机 DDS)     │  │
                                  └──┘
```

### 5.2 进程与主机分布

| 节点/进程 | 运行位置 | 启动方式 |
|---|---|---|
| gst_pipeline | 树莓派 | systemd + bash 脚本 |
| uart_bridge_node | 树莓派 | ros2 launch |
| imu_node（阶段2） | 树莓派 | ros2 launch |
| gst_receiver_node | WSL2 | ros2 launch |
| stereo_splitter_node | WSL2 | ros2 launch |
| detection_node | WSL2 | ros2 launch |
| tracker_node | WSL2 | ros2 launch |
| behavior_node | WSL2 | ros2 launch |
| visual_servo_node | WSL2 | ros2 launch |

### 5.3 连接方式分类

#### 跨主机通信

| 通道 | 协议 | 方向 | QoS |
|---|---|---|---|
| 视频流 | UDP:5600 | 树莓派 → WSL2 | N/A (GStreamer 自管) |
| /servo_cmd | ROS 2 DDS | WSL2 → 树莓派 | SensorDataQoS (best_effort) |
| /servo_state | ROS 2 DDS | 树莓派 → WSL2 | reliable, depth=10 |
| /imu/data | ROS 2 DDS | 树莓派 → WSL2 | SensorDataQoS |

#### 本机通信（节点间，DDS 共享内存优化）

| 通道 | QoS | 备注 |
|---|---|---|
| /stereo/image_raw | SensorDataQoS | 大流量 |
| /camera/image_mono | SensorDataQoS | |
| /detections | reliable, depth=5 | |
| /tracked_objects | reliable, depth=5 | |
| /pixel_error | reliable, depth=1 | 只关心最新 |

#### 硬件接口

| 通道 | 协议 | 方向 |
|---|---|---|
| UART | 921600 bps + CRC16 | 树莓派 ↔ STM32 双向 |
| PWM | 50Hz, 0.5~2.5ms 脉宽 | STM32 → SG90 |
| I2C | 400kHz | 树莓派 → MPU6050（阶段2） |

### 5.4 Service / Action 接口

| 接口 | 类型 | 提供方 | 调用方 | 用途 |
|---|---|---|---|---|
| `/set_target_class` | Service | behavior_node | 外部（CLI / Rviz） | 动态切换跟踪目标 |
| `/reset_target` | Service | behavior_node | 外部 | 清除当前锁定 |
| `/calibrate_center` | Service | visual_servo_node | 外部 | 标定画面中心偏移 |
| `/track_object` (未来) | Action | behavior_node | 外部 | "跟踪 30 秒" 这类长任务 |

---

## 6. 消息与接口定义

### 6.1 使用的标准消息

| 消息类型 | 用途 |
|---|---|
| `sensor_msgs/Image` | 图像话题 |
| `sensor_msgs/JointState` | 舵机命令/状态（name[]=['yaw','pitch','s2','s3'], position[]=角度） |
| `sensor_msgs/Imu` | IMU 数据 |
| `vision_msgs/Detection2DArray` | 检测结果 / 跟踪结果 |
| `geometry_msgs/Vector3` | 像素误差 |

### 6.2 自定义消息（robot_interfaces 包）

```
robot_interfaces/
├── msg/
│   └── TargetInfo.msg           # 当前锁定目标的完整信息
└── srv/
    ├── SetTargetClass.srv
    └── CalibrateCenter.srv
```

#### `TargetInfo.msg`

```
std_msgs/Header header
string class_name           # "cup", "person" etc.
string track_id             # ByteTrack 分配的 ID
float32 confidence
int32 bbox_cx               # 目标中心像素 X
int32 bbox_cy
int32 bbox_w
int32 bbox_h
bool locked                 # 是否已锁定
```

#### `SetTargetClass.srv`

```
string class_name           # COCO 80 类之一，如 "bottle"
---
bool success
string message
```

### 6.3 UART 二进制协议

完整规范见 `docs/uart_protocol.md`，简要：

| 字段 | 长度 | 值 |
|---|---|---|
| HEADER | 2B | `0xAA 0x55` |
| CMD_ID | 1B | `0x01`=舵机控制 / `0x02`=查询 / `0x81`=状态反馈 / `0xFF`=急停 |
| LEN | 1B | payload 长度 |
| PAYLOAD | NB | 见下 |
| CRC16 | 2B | CCITT，覆盖 CMD_ID + LEN + PAYLOAD |
| TAIL | 1B | `0x0D` |

**舵机控制 payload**（CMD_ID=0x01）：

```c
struct ServoCmdItem {
    uint8_t  servo_id;      // 0~3
    int16_t  angle_x10;     // 角度 × 10，如 900 = 90.0°
    uint16_t duration_ms;   // 移动时长
} __attribute__((packed));
// payload = ServoCmdItem × N
```

**状态反馈 payload**（CMD_ID=0x81）：

```c
struct ServoStateItem {
    uint8_t  servo_id;
    int16_t  current_angle_x10;
    uint8_t  status;        // 0=idle, 1=moving, 2=error
} __attribute__((packed));
```

### 6.4 YAML 参数示例

#### `config/visual_servo.yaml`

```yaml
visual_servo_node:
  ros__parameters:
    control_rate_hz: 30.0
    image_width: 320
    image_height: 480
    center_x: 160                 # 画面中心（可通过 /calibrate_center 覆盖）
    center_y: 240
    yaw:
      kp: 0.05
      ki: 0.001
      kd: 0.02
      deadband_px: 5
      output_min: 10.0            # 舵机角度下限
      output_max: 170.0
      max_step_deg: 5.0           # 单次最大增量
    pitch:
      kp: 0.04
      ki: 0.001
      kd: 0.02
      deadband_px: 5
      output_min: 30.0
      output_max: 150.0
      max_step_deg: 5.0
```

---

## 7. 部署与启动顺序

### 7.1 首次部署

**树莓派端**：

```bash
# 1. 环境配置（一次性）
./scripts/setup_rpi.sh

# 2. 构建
cd ros2_ws && colcon build --packages-select \
  robot_interfaces uart_bridge camera_streamer

# 3. 启用 UART（禁用蓝牙占用）
sudo sh -c 'echo "dtoverlay=disable-bt" >> /boot/firmware/config.txt'
sudo systemctl disable hciuart
sudo reboot
```

**WSL2 端**：

```bash
./scripts/setup_wsl2.sh
cd ros2_ws && colcon build
python scripts/export_yolo_onnx.py   # 生成 models/yolov8n.onnx
```

**STM32 端**：用 CubeIDE 烧录 `stm32_fw/` 工程。

### 7.2 运行时启动顺序

**顺序不可颠倒**，否则节点会等待上游话题造成混乱。

```bash
# Terminal 1 [树莓派]：启动相机推流
ssh pi@192.168.10.2
./scripts/start_camera_stream.sh

# Terminal 2 [树莓派]：启动 uart_bridge
ros2 launch uart_bridge uart_bridge.launch.py

# Terminal 3 [WSL2]：启动视觉管道（含所有 WSL2 节点）
export ROS_DOMAIN_ID=42
ros2 launch robot_bringup vision_stack.launch.py

# Terminal 4 [WSL2]：设置目标（可选，默认跟随 person）
ros2 service call /set_target_class \
  robot_interfaces/srv/SetTargetClass "{class_name: 'cup'}"
```

### 7.3 launch 文件层次

```
robot_bringup/launch/
├── vision_stack.launch.py        # WSL2 全套（推荐入口）
│   ├── includes: detection.launch.py
│   ├── includes: tracker.launch.py
│   ├── includes: behavior.launch.py
│   ├── includes: visual_servo.launch.py
│   └── includes: gst_receiver.launch.py
│
├── rpi_stack.launch.py           # 树莓派全套
│   ├── includes: uart_bridge.launch.py
│   └── includes: imu.launch.py （阶段 2）
│
└── full_robot.launch.py          # 如果 ROS 2 能统一启动两边（仅测试用）
```

---

## 8. 开发阶段划分

### 阶段 1：基础链路（1~2 周）

**目标**：每个环节独立验证通过。

| 步骤 | 产出物 | 验证方式 |
|---|---|---|
| 1.1 STM32 固件跑通 | servo_driver + uart_protocol 解析 | 串口助手手动发帧，4 个舵机能动 |
| 1.2 uart_bridge 跑通 | 树莓派 ROS 2 节点 | `ros2 topic pub /servo_cmd ...` 舵机能动 |
| 1.3 GStreamer 管道跑通 | start_camera_stream.sh | WSL2 用 `gst-launch` 拉流能看到图像 |
| 1.4 gst_receiver 跑通 | WSL2 ROS 2 节点 | `rqt_image_view` 能看到 `/stereo/image_raw` |
| 1.5 stereo_splitter 跑通 | | 看到 `/camera/image_mono` |

### 阶段 2：视觉 AI（1~2 周）

| 步骤 | 产出物 | 验证方式 |
|---|---|---|
| 2.1 YOLOv8 离线验证 | 本地 Python 推理能跑 | 视频文件检测效果正常 |
| 2.2 模型导出 ONNX | yolov8n.onnx | 文件生成，大小约 6MB |
| 2.3 detection_node (C++) | ONNX Runtime + CUDA 推理节点 | Rviz 看到 bbox，~15ms/帧 |
| 2.4 tracker_node (ByteTrack) | 集成 ByteTrack C++ 库 | 遮挡后 ID 保持 |

### 阶段 3：控制闭环（1~2 周）⭐ 最难

| 步骤 | 产出物 | 验证方式 |
|---|---|---|
| 3.1 behavior_node | 目标选择逻辑 | `/pixel_error` 曲线合理 |
| 3.2 visual_servo PID 调参 | 完整控制环 | 舵机跟踪物体，无明显振荡 |
| 3.3 STM32 梯形插值 | Traj_Planner_Task | 舵机移动平滑无抖动 |

### 阶段 4：可选增强（后续）

- MPU6050 IMU 接入（稳定平台补偿）
- 双目深度估计（`stereo_depth_node`）
- Action 接口（长任务跟踪）
- TF 坐标变换（相机 → 舵机 → base_link）
- 相机内参标定（提高跟踪精度）

---

## 附录 A：关键技术选型理由

| 选择 | 理由 | 替代方案 |
|---|---|---|
| GStreamer UDP 而非 image_transport | 树莓派有硬件 H.264 编码器，CPU 占用 <5%；端到端延迟 30~50ms | image_transport/compressed (CPU 占用 30%+) |
| ONNX Runtime 而非 PyTorch | C++ 原生、启动快、可与 ROS 2 节点无缝集成 | TensorRT（更快但更复杂） |
| ByteTrack 而非 DeepSORT | 无需额外 ReID 模型、轻量、效果够用 | DeepSORT (抗遮挡更强但慢) |
| 自定义 UART 协议 而非 micro-ROS | STM32F103C8T6 资源不足（F4 以上才推荐 micro-ROS） | micro-ROS |
| vision_msgs 而非自定义消息 | ROS 2 官方标准，Rviz 有原生插件 | 自定义 msg |
| Win11 mirrored 网络 | 跨主机 DDS 零配置 | Fast DDS Discovery Server |

## 附录 B：已知限制与未来改进

| 当前限制 | 影响 | 未来改进 |
|---|---|---|
| SG90 精度差（死区 ~5°） | 跟踪精度上限 | 换数字舵机（MG996R+） |
| 单目检测，无深度 | 不能判断物体远近 | 加 stereo_depth_node |
| 无 TF 坐标系 | 机器人不知道自身姿态 | 引入 TF 树 + IMU 融合 |
| 无相机标定 | 画面中心可能偏离光轴 | 用 camera_calibration 包 |
| 固定目标类别 | 无法语音切换 | 集成 whisper + LLM 意图识别 |
| 无持久化状态 | 重启后无记忆 | 加入 SQLite 记录跟踪历史 |

---

**文档结束**
