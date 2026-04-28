# 物体识别与跟随机器人 · 架构文档

**项目名称**：`desktop_tracking_robot`  
**架构版本**：v3.0  
**最后更新**：2026-04-28  
**完成度**：~87.5%（等待硬件接线）

---

## 目录

1. [系统概述](#1-系统概述)
2. [硬件层次与通信链路](#2-硬件层次与通信链路)
3. [代码仓库结构](#3-代码仓库结构)
4. [核心模块详解](#4-核心模块详解)
5. [数据流与节点图](#5-数据流与节点图)
6. [消息接口定义](#6-消息接口定义)
7. [UART 二进制协议](#7-uart-二进制协议)
8. [外部依赖](#8-外部依赖)
9. [构建与启动](#9-构建与启动)

---

## 1. 系统概述

本项目是一个基于三层分布式架构的桌面目标跟随机器人：

- **PC（WSL2）**：运行 AI 推理（YOLOv8）、目标跟踪（ByteTrack）和视觉引导腿部控制（PID + 简化步态）
- **树莓派 4B**：接入双目摄像头、推送 H.264 视频流、桥接 ROS 2 与 STM32 UART
- **STM32F103C8T6**：FreeRTOS 实时控制，驱动 4 路 SG90 舵机（梯形轨迹规划 + PWM 输出）

整个系统的数据流：**摄像头 → 视频流 → YOLOv8 检测 → ByteTrack 跟踪 → PID 控制 → UART → STM32 → 舵机**。

---

## 2. 硬件层次与通信链路

```
┌──────────────────────────────────────────────────┐
│               PC (Win11 + WSL2)                  │
│  Ubuntu 24.04 + NVIDIA GPU (CUDA)                │
│  IP: 192.168.10.1                                │
│                                                  │
│  ┌───────────────────────────────────────────┐   │
│  │ ROS 2 Jazzy (DDS)                         │   │
│  │ gst_receiver → stereo_splitter            │   │
│  │   → detection_node → tracker_node         │   │
│  │   → behavior_node → leg_motion_node       │   │
│  └───────────────────────┬───────────────────┘   │
└──────────────────────────┼───────────────────────┘
           ROS 2 DDS (UDP) │ /servo_cmd →
           (mirrored 网络) │ /servo_state ←
┌──────────────────────────┼───────────────────────┐
│          树莓派 4B         │                       │
│  IP: 192.168.10.2         │                       │
│  ┌────────────────────────▼──────────────────┐   │
│  │ uart_bridge_node (ROS 2)                  │   │
│  │ GStreamer 推流进程 (非 ROS)                │   │
│  └──────────────────┬─────────────────────────┘  │
└─────────────────────┼──────────────────────────── ┘
         GPIO UART    │ 921600 bps + CRC16
         /dev/ttyAMA0 │ (ServoCmdItem / ServoStateItem_v2)
┌─────────────────────┼────────────────────────────┐
│  STM32F103C8T6      │                            │
│  FreeRTOS           │                            │
│  ┌──────────────────▼──────────────────────────┐ │
│  │ uart_rx_task → traj_planner_task            │ │
│  │ → servo_driver (TIM2 PWM)                   │ │
│  │ → status_safety_task                        │ │
│  └─────────────────────────────────────────────┘ │
│                      PWM 50Hz                    │
│           → SG90 × 4 (front_left + front_right + rear_left + rear_right) │
└───────────────────────────────────────────────────┘
```

### 2.1 通信协议矩阵

| 链路 | 协议 | 方向 | 参数 |
|---|---|---|---|
| 树莓派 → PC | H.264 over GStreamer UDP | 单向 | port 5600, ~30fps |
| PC ↔ 树莓派 | ROS 2 DDS（CycloneDDS） | 双向 | Domain ID=0（默认） |
| 树莓派 ↔ STM32 | UART 二进制帧 | 双向 | 921600bps, 8N1, CRC16 |
| STM32 → SG90 | PWM | 单向 | 50Hz, 0.5~2.5ms 脉宽 |

---

## 3. 代码仓库结构

```
dog_dev/
├── CLAUDE.md                          # Claude Code 使用约定
├── ARCHITECTURE.md                    # 本文档（实际代码结构）
├── README.md                          # 快速入门
├── SETUP_GUIDE.md                     # 硬件接线指南
├── QUICK_START.md                     # 启动步骤
├── architecture.md                    # 原始架构设计文档（设计阶段）
├── task.md                            # 40 个开发任务清单
├── progress.md                        # 项目进度记录
│
├── ros2_ws/                           # ROS 2 工作空间
│   └── src/
│       ├── robot_interfaces/          # 自定义消息/服务定义
│       │   ├── msg/
│       │   │   ├── SimpleDetection.msg          # 单个检测框（含 track_id）
│       │   │   ├── SimpleDetection2DArray.msg   # 检测框数组（带 Header）
│       │   │   └── TargetInfo.msg               # 跟踪目标完整信息
│       │   └── srv/
│       │       ├── SetTargetClass.srv           # 动态切换跟踪目标类别
│       │       └── CalibrateCenter.srv          # 标定画面中心偏移
│       │
│       ├── gst_receiver/              # [WSL2] GStreamer UDP 接收节点
│       │   ├── include/gst_receiver/gst_receiver_node.hpp
│       │   ├── src/gst_receiver_node.cpp        # appsink 解码 → ROS 图像
│       │   └── src/gst_receiver_main.cpp
│       │
│       ├── stereo_splitter/           # [WSL2] 双目图像分割节点
│       │   ├── include/stereo_splitter/stereo_splitter_node.hpp
│       │   └── src/stereo_splitter_node.cpp     # 640×480 → 左 320×480
│       │
│       ├── detection_node/            # [WSL2] YOLOv8 目标检测节点 ⭐
│       │   ├── include/detection_node/
│       │   │   ├── detection_node.hpp     # ROS 2 节点类（生产者-消费者队列）
│       │   │   ├── yolo_infer.hpp         # ONNX Runtime 推理封装
│       │   │   └── detection_viz_node.hpp # 可视化节点（调试用）
│       │   └── src/
│       │       ├── detection_node.cpp     # 图像入队 + 推理线程
│       │       ├── yolo_infer.cpp         # letterbox + 推理 + NMS
│       │       └── detection_node_main.cpp
│       │
│       ├── tracker_node/              # [WSL2] ByteTrack 目标跟踪节点 ⭐
│       │   ├── include/tracker_node/
│       │   │   ├── tracker_node.hpp       # ROS 2 节点类（消息格式转换）
│       │   │   └── byte_tracker.hpp       # ByteTrack 算法（贪心 IoU 匹配）
│       │   ├── src/
│       │   │   ├── tracker_node.cpp       # 订阅/发布 + 坐标格式转换
│       │   │   └── byte_tracker.cpp       # 轨迹管理（创建/更新/删除）
│       │   └── test/
│       │       └── test_byte_tracker.cpp
│       │
│       ├── behavior_node/             # [WSL2] 行为决策节点
│       │   ├── include/behavior_node/behavior_node.hpp
│       │   └── src/
│       │       ├── behavior_node.cpp      # 目标筛选 + 像素误差计算
│       │       └── main.cpp
│       │
│       ├── visual_servo/              # [WSL2] 视觉引导腿部控制节点 ⭐
│       │   ├── include/visual_servo/
│       │   │   ├── visual_servo_node.hpp  # 30Hz 四腿控制环节点
│       │   │   └── pid_controller.hpp     # 单轴 PID（死区 + 积分饱和）
│       │   └── src/
│       │       ├── visual_servo_node.cpp  # 误差 → 前进/转向 → 四腿舵机角度命令
│       │       ├── pid_controller.cpp     # PID 算法实现
│       │       └── main.cpp
│       │
│       ├── uart_bridge/               # [树莓派] UART ↔ ROS 2 桥接节点 ⭐
│       │   ├── include/uart_bridge/
│       │   │   ├── uart_protocol.h        # 协议帧定义（共享头文件 C/C++ 兼容）
│       │   │   ├── frame_parser.hpp       # 字节流 → 帧（8 状态状态机）
│       │   │   └── frame_encoder.hpp      # 帧结构 → 字节流（CRC16 计算）
│       │   └── src/
│       │       ├── uart_bridge_node.cpp   # 主节点（含时间戳映射/延迟监控/握手）
│       │       ├── frame_parser.cpp       # 状态机实现
│       │       ├── frame_encoder.cpp      # 帧构建实现
│       │       └── uart_protocol.c        # CRC16-CCITT 算法
│       │
│       └── robot_bringup/             # Launch 文件集合
│           └── launch/
│               ├── vision_stack.launch.py     # WSL2 全节点启动入口
│               ├── rpi_stack.launch.py        # 树莓派全节点启动入口
│               └── test_73_complete.launch.py # 完整系统集成测试
│
├── stm32_keil/                        # STM32 固件工程（Keil MDK）
│   └── Core/
│       ├── Inc/
│       │   ├── uart_protocol.h        # ← 与 uart_bridge 共用同一协议头
│       │   ├── servo_driver.h         # TIM2 PWM 舵机驱动接口
│       │   ├── traj_planner.h         # 梯形轨迹规划接口
│       │   ├── uart_rx_task.h         # UART DMA 接收任务接口
│       │   ├── status_safety_task.h   # 安全监控（超时保护 + IWDG）接口
│       │   └── FreeRTOSConfig.h       # FreeRTOS 配置（CMSIS-V2）
│       └── Src/
│           ├── main.c                 # FreeRTOS 启动入口 + 外设初始化
│           ├── servo_driver.c         # TIM2 PWM 输出（4 路，0~180°）
│           ├── traj_planner.c         # 梯形速度曲线插值（5ms 粒度）
│           ├── uart_rx_task.c         # DMA+IDLE 中断接收 + CRC 校验 + 解析
│           ├── status_safety_task.c   # 舵机状态上报（50ms）+ 超时保护
│           ├── uart_protocol.c        # CRC16 计算（与 PC 端相同算法）
│           └── freertos.c             # FreeRTOS 任务创建入口
│
├── models/                            # AI 模型文件
│   ├── yolov8n.onnx                   # YOLOv8n 导出，640×640 输入，6MB
│   └── coco_classes.txt               # 80 类 COCO 类别名称（行号=class_id）
│
├── config/                            # ROS 2 节点运行时参数
│   ├── visual_servo.yaml              # PID 参数（Kp/Ki/Kd、死区、限幅）
│   └── behavior.yaml                  # 图像尺寸 + 默认目标类别
│
├── shared/                            # 跨平台共享代码
│   └── uart_protocol.h                # 协议头，与 stm32_keil/Core/Inc/ 同步
│
├── tests/                             # 离线单元测试
│   ├── test_uart_frame_codec.cpp      # 编/解码往返测试（含 CRC 破坏测试）
│   ├── test_uart_frame_parser.cpp     # 解析器状态机单元测试
│   ├── test_uart_encode_send.cpp      # 编码发送模拟测试
│   ├── test_pid_controller.cpp        # PID 控制器单元测试（8 个测试用例）
│   ├── test_byte_tracker.cpp          # ByteTracker 单元测试（8 个测试用例）
│   └── test_detection/
│       ├── test_onnx_cpp.cpp          # C++ ONNX Runtime 推理功能测试
│       ├── test_yolo_gpu.py           # Python GPU 推理性能测试
│       └── test_onnx_python.py        # Python ONNX Runtime 正确性测试
│
├── scripts/                           # 辅助脚本（编译/启动/配置）
└── docs/                              # 额外文档
    ├── uart-packet-loss.md            # UART 丢包分析报告
    ├── ros2-topology.md               # ROS 2 节点拓扑图
    └── dds-network.md                 # WSL2 + CycloneDDS 网络配置说明
```

---

## 4. 核心模块详解

### 4.1 `detection_node` — 目标检测 ⭐

**文件**：`ros2_ws/src/detection_node/`  
**运行位置**：WSL2（需要 NVIDIA GPU + CUDA）  
**依赖**：ONNX Runtime 1.17、CUDA、OpenCV 4.x

#### `DetectionNode`（`detection_node.hpp/cpp`）

ROS 2 节点，采用**生产者-消费者**模式将图像接收与推理解耦：

- **订阅**：`/camera/image_mono` (sensor_msgs/Image, 320×480)
- **发布**：`/detections` (robot_interfaces/SimpleDetection2DArray)
- `ImageCallback()`：图像入队（最大 2 帧），避免推理堵塞订阅回调
- `InferenceWorker()`：单独线程，阻塞等待队列非空，逐帧推理

#### `YoloInfer`（`yolo_infer.hpp/cpp`）

ONNX Runtime 推理封装：

1. **`Letterbox()`**：保持宽高比将输入缩放至 640×640，灰色填充（114,114,114），记录 `scale` 和 `pad_x/pad_y` 用于后处理坐标还原
2. **`Infer()`**：BGR→RGB→CHW（HWC 转换）→ 归一化至 [0,1] → ONNX 推理  
   输出 shape `[1, 84, 8400]` 转置为 `[8400, 84]`
3. **`PostProcess()`**：置信度过滤（0.25）→ de-letterbox 坐标还原 → 生成检测列表
4. **`NMS()`**：非极大值抑制（IoU 阈值 0.45），按置信度降序排列

**性能**：~15ms/帧（GPU），目标检测置信度阈值 0.5（objectness × max_class_prob）

---

### 4.2 `tracker_node` — 目标跟踪 ⭐

**文件**：`ros2_ws/src/tracker_node/`  
**运行位置**：WSL2

#### `ByteTracker`（`byte_tracker.hpp/cpp`）

简化版 ByteTrack 算法：

- **数据结构**：`active_tracks_` = `std::map<int, TrackState>`，每个 TrackState 包含 `track_id`、`last_detection`（bbox）、`miss_frames`、`active_frames`
- **匹配策略**：以 `1 - IoU` 为代价，**贪心分配**（非匈牙利算法）
- **生命周期**：
  - 未匹配检测 → 创建新轨迹（`next_id_++`）
  - 未匹配轨迹 → `miss_frames++`
  - `miss_frames > track_buffer`（默认 30）→ 删除轨迹
- **置信度过滤**：`conf < track_thresh`（默认 0.5）的检测不参与匹配

#### `TrackerNode`（`tracker_node.hpp/cpp`）

消息格式转换：`SimpleDetection2DArray` ↔ `tracker_node::Detection`  
为每个被跟踪目标的 `track_id` 字段填写字符串 ID，再发布到 `/tracked_objects`。

---

### 4.3 `behavior_node` — 行为决策

**文件**：`ros2_ws/src/behavior_node/`  
**运行位置**：WSL2

**核心逻辑**（`OnTrackedObjects()` + `SelectTarget()`）：

1. 遍历 `/tracked_objects` 中所有检测，过滤 `class_id == target_class_id_`
2. 选面积最大（`width × height`）的目标
3. 计算像素误差：`error_x = center_x - 160`，`error_y = center_y - 240`
4. 发布 `geometry_msgs/Vector3` 到 `/pixel_error`

**服务** `/set_target_class`：运行时通过类别名字符串（如 `"person"`、`"cup"`）切换跟踪目标，自动查 `coco_classes.txt` 映射为 class_id

---

### 4.4 `visual_servo` — 视觉引导腿部控制 ⭐

**文件**：`ros2_ws/src/visual_servo/`  
**运行位置**：WSL2

#### `PIDController`（`pid_controller.hpp/cpp`）

单轴 PID，三个保护机制：
- **死区**（deadband）：误差绝对值 < deadband 时误差归零，防止目标在画面中心附近时的舵机微抖动
- **积分饱和**（anti-windup）：积分项钳制在 `[-output_max/2, output_max/2]`，防止长时间稳态误差时积分无限累积
- **输出钳制**：最终输出限制在 `[output_min, output_max]`

#### `LegMotionControllerNode`（`visual_servo_node.hpp/cpp`）

30Hz 定时器控制环（`OnControlTimer()`）：

```
dt = now - last_control_time_
turn_error    = -pixel_error_x
forward_error = -pixel_error_y
turn_bias     = pid_turn_.Update(turn_error, dt)
stride_cmd    = pid_forward_.Update(forward_error, dt)
phase         = gait_frequency_hz * dt
四条腿角度 = neutral_angle ± gait(phase) ± turn_bias
发布 /servo_cmd (JointState, 弧度单位)
```

固定摄像头不转动；`/servo_state` 用来回读四条腿当前角度（弧度→度）。

---

### 4.5 `uart_bridge` — UART 协议桥接 ⭐

**文件**：`ros2_ws/src/uart_bridge/`  
**运行位置**：树莓派

| 类 | 文件 | 职责 |
|---|---|---|
| `FrameParser` | `frame_parser.hpp/cpp` | 字节流 → 完整帧（8 状态状态机，含 CRC 校验） |
| `FrameEncoder` | `frame_encoder.hpp/cpp` | 帧结构 → 字节流（CRC16 小端序） |
| `TimestampMapper` | `uart_bridge_node.cpp` | STM32 ms 时间戳 ↔ ROS 时间线性插值（最近 20 个映射点） |
| `LatencyMonitor` | `uart_bridge_node.cpp` | 记录 UART 传输延迟统计（min/max/avg，100 个样本） |
| `FrameSequenceChecker` | `uart_bridge_node.cpp` | 逐舵机序列号连续性检测（检测丢帧） |
| `UartBridgeNode` | `uart_bridge_node.cpp` | 主节点：独立读线程 + ROS 2 订阅/发布 + 握手状态机 |

**启动握手流程**：
```
上电 → 发送 INIT_HANDSHAKE → 每 500ms 重试
     → 收到 SYSTEM_STATE (system_state == ACTIVE) → 握手完成
     → 握手完成前丢弃所有 /servo_cmd（避免 STM32 初始化中接收指令）
```

**数据路径**（接收方向）：
```
ReadLoop (独立线程) → parser_->ProcessByte() 
→ OnFrameReceived() → 解析 ServoStateItem_v2
→ TimestampMapper 时间戳映射
→ LatencyMonitor 延迟记录
→ FrameSequenceChecker 丢帧检测
→ 发布 /servo_state
```

---

### 4.6 STM32 固件 — 实时舵机控制

**文件**：`stm32_keil/Core/`  
**硬件**：STM32F103C8T6，64KB Flash / 20KB RAM，FreeRTOS CMSIS-V2

#### FreeRTOS 任务

| 任务 | 文件 | 周期 | 优先级 |
|---|---|---|---|
| `uart_rx_task` | `uart_rx_task.c` | 中断驱动（DMA+IDLE） | High |
| `TrajPlannerTask` | `traj_planner.c` | 5ms | AboveNormal |
| `Status_Safety_Task` | `status_safety_task.c` | 50ms（上报）/ 100ms（安全检查） | Normal |

#### 梯形速度曲线（`traj_planner.c`）

```
总时间 T，位移 D
加速段（0 ≤ t ≤ t1 = 0.3T）：angle = start + 0.5·a·t²
匀速段（t1 < t ≤ t2 = 0.7T）：angle = 加速段末 + v_max·(t-t1)
减速段（t2 < t ≤ T）：         angle = 匀速段末 + v_max·τ - 0.5·a·τ²（τ = t-t2）

v_max = D / (T × 0.7)
a = v_max / (T × 0.3)
```

#### PWM 计算（`servo_driver.c`）

```c
CCR = 500 + angle_deg × (2000.0f / 180.0f)
// 0°   → CCR = 500  → 0.5ms → TIM2 计数器基于 1MHz 时钟
// 90°  → CCR = 1500 → 1.5ms
// 180° → CCR = 2500 → 2.5ms
```

---

## 5. 数据流与节点图

### 5.1 完整数据流

```
[USB 双目摄像头] /dev/video0 (640×480 YUYV)
    │
    ▼ GStreamer 推流进程（树莓派，非 ROS）
    │ H.264 UDP:5600
    ▼（跨主机网络）
[gst_receiver_node]       /stereo/image_raw (640×480)
    ▼
[stereo_splitter_node]    /camera/image_mono (320×480，左半图)
    ▼
[detection_node] CUDA     /detections (SimpleDetection2DArray)
                          ← {center_x, center_y, w, h, class_id, confidence}
    ▼
[tracker_node]            /tracked_objects (SimpleDetection2DArray)
                          ← 同上，增加 track_id 字符串
    ▼
[behavior_node] ◄── /set_target_class (service)
                          /pixel_error (Vector3)
                          ← x = target_cx - 160, y = target_cy - 240
    ▼
[leg_motion_node] 30Hz   /servo_cmd (JointState)
    ▲ /servo_state        ← name=["front_left","front_right","rear_left","rear_right"], position=[rad...]
    │（跨主机 ROS 2 DDS）
    ▼
[uart_bridge_node]（树莓派）
    │ UART 二进制帧 921600bps
    ▼
[STM32 uart_rx_task]
    ▼
[TrajPlannerTask] 5ms     梯形轨迹插值
    ▼
[servo_driver]            TIM2_CH1~CH4 PWM 50Hz
    ▼
[SG90 × 4]
```

### 5.2 话题/服务汇总

| 话题/服务 | 类型 | 发布方 | 订阅方 | QoS |
|---|---|---|---|---|
| `/stereo/image_raw` | sensor_msgs/Image | gst_receiver | stereo_splitter | SensorDataQoS |
| `/camera/image_mono` | sensor_msgs/Image | stereo_splitter | detection_node | SensorDataQoS |
| `/detections` | SimpleDetection2DArray | detection_node | tracker_node | depth=10 |
| `/tracked_objects` | SimpleDetection2DArray | tracker_node | behavior_node | reliable, depth=5 |
| `/pixel_error` | geometry_msgs/Vector3 | behavior_node | leg_motion_node | depth=5 |
| `/servo_cmd` | sensor_msgs/JointState | leg_motion_node | uart_bridge_node | SensorDataQoS |
| `/servo_state` | sensor_msgs/JointState | uart_bridge_node | leg_motion_node | reliable, depth=10 |
| `/set_target_class` | SetTargetClass (srv) | — | behavior_node | — |

---

## 6. 消息接口定义

### 6.1 `SimpleDetection.msg`

```
float32 center_x      # 检测框中心 X 像素坐标
float32 center_y      # 检测框中心 Y 像素坐标
float32 width         # 检测框宽度（像素）
float32 height        # 检测框高度（像素）
int32   class_id      # COCO 类别 ID（0~79，行号对应 coco_classes.txt）
float32 confidence    # 置信度（objectness × max_class_prob，0~1）
string  track_id      # ByteTrack 分配的 ID 字符串（检测阶段为空字符串）
```

### 6.2 `SetTargetClass.srv`

```
string class_name    # COCO 类别名，如 "person"、"cup"、"bottle"
---
bool success
string message
```

---

## 7. UART 二进制协议

协议版本：v3（`UART_PROTOCOL_VERSION = 0x03`）

### 7.1 帧格式

```
字节位置:  [0]   [1]   [2]     [3]   [4..4+N-1]  [4+N]    [5+N]    [6+N]
内容:     0xAA  0x55  CMD_ID  LEN   PAYLOAD...  CRC_LO   CRC_HI   0x0D
大小:       1B    1B    1B      1B      N×B         1B       1B       1B
```

- **CRC 算法**：CRC16-CCITT（多项式 0x1021），覆盖 `[CMD_ID, LEN, PAYLOAD...]`
- **CRC 字节序**：小端（低字节 CRC_LO 先发，与 STM32 大端 uint16 存储一致）
- **最大帧长**：256 字节

### 7.2 命令 ID 定义

| CMD_ID | 名称 | 方向 | 说明 |
|---|---|---|---|
| `0x01` | SERVO_CONTROL | 树莓派 → STM32 | 设置舵机目标角度 + 运动时长 |
| `0x02` | QUERY | 树莓派 → STM32 | 查询当前状态 |
| `0x10` | INIT_HANDSHAKE | 树莓派 → STM32 | 建立连接握手（含协议版本校验） |
| `0x81` | SERVO_STATE | STM32 → 树莓派 | 舵机状态 v1（不含时间戳） |
| `0x82` | SERVO_STATE_V2 | STM32 → 树莓派 | 舵机状态 v2（含 STM32 时间戳 + 序列号） |
| `0x83` | SYSTEM_STATE | STM32 → 树莓派 | 系统状态（启动/就绪/错误 + uptime） |
| `0xFF` | EMERGENCY_STOP | 双向 | 紧急停止，所有舵机停止运动 |

### 7.3 关键数据结构

```c
// 舵机控制指令（一帧可包含多个，N = count × sizeof(ServoCmdItem)）
typedef struct __attribute__((packed)) {
    uint8_t  servo_id;       // 0=front_left, 1=front_right, 2=rear_left, 3=rear_right
    int16_t  angle_x10;      // 角度 × 10，如 900 代表 90.0°
    uint16_t duration_ms;    // 期望运动时长（ms），用于梯形规划
} ServoCmdItem;              // sizeof = 5 字节

// 舵机状态反馈 v2（含 STM32 时间戳，用于延迟测量）
typedef struct __attribute__((packed)) {
    uint8_t  servo_id;
    int16_t  current_angle_x10;   // 当前角度 × 10
    uint8_t  status;              // 0=idle, 1=moving
    uint16_t timestamp_ms;        // STM32 HAL_GetTick() 时间戳
    uint16_t frame_seq;           // 帧序列号（用于丢帧检测）
} ServoStateItem_v2;             // sizeof = 8 字节

// 握手 payload（CMD_ID = 0x10）
typedef struct __attribute__((packed)) {
    uint8_t  protocol_version;   // 必须等于 UART_PROTOCOL_VERSION(0x03)
    uint8_t  requested_state;    // 请求 STM32 进入 ACTIVE 状态
    uint16_t reserved;           // 保留，填 0
} UartHandshakePayload;         // sizeof = 4 字节

// 系统状态 payload（CMD_ID = 0x83，STM32 → 树莓派）
typedef struct __attribute__((packed)) {
    uint8_t  protocol_version;
    uint8_t  system_state;       // UartSystemState 枚举值
    uint16_t reserved;
    uint32_t uptime_ms;          // STM32 运行时间（HAL_GetTick()）
} UartSystemStatePayload;       // sizeof = 8 字节
```

### 7.4 STM32 状态机

```
上电初始化
    │
    ▼ BOOT_CENTERING (0x01)
    │  舵机归中（90°），等待完成
    │
    ▼ WAITING_CONNECTION (0x02)
    │  等待树莓派发送 INIT_HANDSHAKE
    │  每 50ms 上报 SYSTEM_STATE
    │
    ▼ ACTIVE (0x03)
    │  可正常接收 SERVO_CONTROL 指令
    │  每 50ms 上报 SERVO_STATE_V2
    │
    ▼ ERROR (0x7F)
       故障状态（IWDG 超时重启）
```

---

## 8. 外部依赖

### 8.1 WSL2（Ubuntu 24.04）

| 库/工具 | 版本 | 用途 | 安装路径 |
|---|---|---|---|
| ROS 2 Jazzy | stable | 节点通信框架 | `/opt/ros/jazzy` |
| ONNX Runtime (GPU) | 1.17 | YOLOv8 推理 | `/home/peter/onnxruntime-gpu`（CMake 硬编码） |
| CUDA | ≥11.x | GPU 推理加速 | 系统安装 |
| OpenCV | 4.x | 图像预处理 | 系统包 |
| GStreamer | 1.0 | H.264 视频流解码 | 系统包 + `gstreamer1.0-plugins-*` |
| CycloneDDS | ROS 2 默认 | DDS 通信，支持 Win11 mirrored 网络 | ROS 2 内置 |

### 8.2 树莓派 4B（Ubuntu Server 24.04）

| 库/工具 | 版本 | 用途 |
|---|---|---|
| ROS 2 Jazzy | stable | uart_bridge_node 运行环境 |
| GStreamer | 1.0 | v4l2 摄像头采集 + H.264 编码 + UDP 推流 |
| POSIX termios | 系统库 | UART 串口配置（921600 bps, 8N1） |

### 8.3 STM32F103C8T6

| 库 | 来源 | 用途 |
|---|---|---|
| FreeRTOS | CMSIS-V2（CubeMX 生成） | 实时任务调度（3 个任务） |
| STM32 HAL | CubeMX 自动生成 | GPIO/UART/TIM/DMA/IWDG 驱动 |

### 8.4 Python 开发工具

| 工具 | 用途 |
|---|---|
| `ultralytics` | YOLOv8 模型训练/导出 ONNX |
| `onnxruntime-gpu` | Python 推理测试脚本 |
| `opencv-python` | 图像处理脚本 |

---

## 9. 构建与启动

### 9.1 WSL2 编译顺序

```bash
cd ros2_ws
# 先编译接口包（其他包依赖它）
colcon build --packages-select robot_interfaces

# 再编译节点包
colcon build --packages-select \
  gst_receiver stereo_splitter \
  detection_node tracker_node \
  behavior_node visual_servo
```

> **注意**：`detection_node` 的 CMakeLists.txt 硬编码了  
> `set(ONNXRUNTIME_ROOT /home/peter/onnxruntime-gpu)`  
> 如在其他机器编译，需修改此路径。

### 9.2 树莓派编译

```bash
cd ros2_ws
colcon build --packages-select robot_interfaces uart_bridge
```

### 9.3 STM32 编译

使用 **Keil MDK** 打开 `stm32_keil/` 工程，直接编译并通过 ST-Link 烧录。

### 9.4 运行顺序

```bash
# 步骤 1：STM32 上电（FreeRTOS 自动启动，舵机归中）

# 步骤 2：树莓派 - 启动 GStreamer 摄像头推流
# （见 scripts/ 下的推流脚本）

# 步骤 3：树莓派 - 启动 UART 桥接节点
ros2 launch robot_bringup rpi_stack.launch.py

# 步骤 4：WSL2 - 启动视觉管道（所有 WSL2 节点）
ros2 launch robot_bringup vision_stack.launch.py

# 步骤 5（可选）：动态切换跟踪目标类别
ros2 service call /set_target_class \
  robot_interfaces/srv/SetTargetClass "{class_name: 'bottle'}"
```

### 9.5 离线测试（无 ROS 2 环境）

```bash
cd tests

# UART 帧编解码往返测试
g++ test_uart_frame_codec.cpp \
    -I../ros2_ws/src/uart_bridge/include \
    ../ros2_ws/src/uart_bridge/src/frame_parser.cpp \
    ../ros2_ws/src/uart_bridge/src/frame_encoder.cpp \
    ../ros2_ws/src/uart_bridge/src/uart_protocol.c \
    -o test_uart_frame_codec && ./test_uart_frame_codec

# PID 控制器单元测试
g++ test_pid_controller.cpp \
    -I../ros2_ws/src/visual_servo/include \
    ../ros2_ws/src/visual_servo/src/pid_controller.cpp \
    -o test_pid_controller && ./test_pid_controller

# ByteTracker 单元测试
g++ test_byte_tracker.cpp \
    -I../ros2_ws/src/tracker_node/include \
    ../ros2_ws/src/tracker_node/src/byte_tracker.cpp \
    -o test_byte_tracker && ./test_byte_tracker
```

---

*文档更新于 2026-04-28 · 对应实际代码状态*
