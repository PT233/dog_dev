# 系统拓扑

基线日期：`2026-04-28`

## 1. 三个运行域

| 域 | 主要实体 | 作用 |
| --- | --- | --- |
| WSL2 / PC | `gst_receiver_node`、`stereo_splitter_node`、`detection_node`、`tracker_node`、`behavior_node`、`leg_motion_node` | 视觉处理、目标选择、运动命令生成 |
| Raspberry Pi | `start_camera_stream.sh`、`uart_bridge_node` | 视频推流、UART 桥接 |
| STM32F103 | `Task_UART_RX`、`Task_Traj_Planner`、`Task_Status_TX`、`Task_Safety` | 实时舵机控制与状态回传 |

## 2. 总链路

```mermaid
flowchart LR
  subgraph WSL["WSL2 / PC"]
    GR[gst_receiver_node]
    SS[stereo_splitter_node]
    DN[detection_node]
    TN[tracker_node]
    BN[behavior_node]
    VS[leg_motion_node]
  end

  subgraph RPI["Raspberry Pi"]
    CAM[/dev/video0]
    GST[start_camera_stream.sh]
    UB[uart_bridge_node]
  end

  subgraph STM["STM32F103"]
    RX[Task_UART_RX]
    TP[Task_Traj_Planner]
    TX[Task_Status_TX]
    SF[Task_Safety]
    PWM[TIM2 PWM]
    SERVO[SG90 x4]
  end

  CAM --> GST
  GST -->|UDP H.264 :5600| GR
  GR -->|/stereo/image_raw| SS
  SS -->|~/input/image| DN
  DN -->|/detection_node/output/detections| TN
  TN -->|/tracker_node/output/tracked_objects| BN
  BN -->|/behavior_node/output/pixel_error| VS
  VS -->|/leg_motion_node/output/servo_command| UB
  UB -->|/uart_bridge_node/output/servo_state| VS
  UB -->|UART 0x01 / 0x10| RX
  TX -->|UART 0x82 / 0x83| UB
  RX --> TP
  TP --> PWM
  PWM --> SERVO
  SF --> TX
```

## 3. 关键物理连接

| 两端 | 链路 |
| --- | --- |
| 摄像头 -> Pi | USB / V4L2 |
| Pi -> WSL2 | `UDP/H.264 :5600` |
| WSL2 <-> Pi | ROS 2 DDS |
| Pi <-> STM32 | UART `/dev/ttyAMA0` <-> `USART1`, `921600 8N1` |
| STM32 -> SG90 | `TIM2 CH1~CH4`, 50 Hz PWM |

## 4. 当前边界说明

- 默认主链不包含 `detection_viz_node`
- `vision_front` 是实验入口，不是标准部署链路
- `CalibrateCenter.srv` 目前未接入任何服务端
