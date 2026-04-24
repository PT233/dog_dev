# 物体识别跟随机器人 🤖

> 基于 YOLO + ByteTrack 的实时双目视觉目标跟踪系统

**Version**: 3.0 | **Status**: 功能完整，调试就绪 | **Last Updated**: 2026-04-24

Desktop tracking robot for real-time object detection and servo following with ROS 2, Raspberry Pi, and STM32.

---

## 📖 快速导航

| 文档 | 内容 | 适用人群 |
|---|---|---|
| **[SETUP_GUIDE.md](SETUP_GUIDE.md)** | 📚 完整设置指南（硬件→软件→调试→应用） | 首次接触该项目 |
| **[QUICK_START.md](QUICK_START.md)** | 🚀 快速启动命令（硬件就绪后） | 硬件已接线 |
| **[hardware_wiring.html](hardware_wiring.html)** | 🔌 交互式接线图（STM32、树莓派、舵机） | 进行硬件接线 |
| **[architecture.md](architecture.md)** | 🏗️ 系统架构设计 | 理解整体设计 |
| **[task.md](task.md)** | 📋 40 个开发任务清单 | 开发参考 |
| **[progress.md](progress.md)** | 📊 项目进度和阻塞记录 | 跟踪进度 |

---

## ⚠️ 当前状态

**🔴 阶段 7.3-7.6 阻塞**：硬件接线尚未完成

- ✅ 阶段 0-6：所有代码和配置已完成（35/40 任务）
- ✅ 阶段 7.1-7.2：Launch 文件已创建
- ⏸️ 阶段 7.3-7.6：等待硬件接线完成后才能进行端到端测试

**立即开始**：👉 请查看 [SETUP_GUIDE.md](SETUP_GUIDE.md) 中的硬件接线部分

---

## 📦 项目结构

```
├── SETUP_GUIDE.md              ← 完整设置指南
├── QUICK_START.md              ← 快速启动命令  
├── hardware_wiring.html        ← 交互式接线图
├── architecture.md             ← 系统架构
├── task.md                     ← 任务清单
├── progress.md                 ← 项目进度
│
├── ros2_ws/src/                ← ROS 2 源代码
│   ├── robot_bringup/          ← 启动脚本
│   ├── gst_receiver/           ← 视频接收
│   ├── detection_node/         ← YOLO 检测
│   ├── tracker_node/           ← 目标跟踪
│   ├── behavior_node/          ← 决策层
│   ├── visual_servo/           ← PID 控制
│   └── uart_bridge/            ← 串口桥接
│
├── stm32_fw/                   ← STM32 固件
├── config/                     ← 配置文件
├── models/                     ← AI 模型
├── scripts/                    ← 辅助脚本
└── docs/                       ← 文档（待补充）
```

---

## 🎯 核心功能

✅ **视觉识别**：YOLOv8n 实时检测 COCO 80 类物体  
✅ **目标跟踪**：ByteTrack 算法抗遮挡关联  
✅ **视觉伺服**：PID 控制舵机平滑跟随  
✅ **分布式系统**：PC(WSL2) + 树莓派 + STM32 协作  
✅ **硬实时控制**：FreeRTOS 梯形速度规划  

---

## 🚀 三步启动（硬件就绪后）

```bash
# Terminal 1: 树莓派相机
ssh ubuntu@192.168.137.100
./scripts/start_camera_stream.sh

# Terminal 2: 树莓派 ROS
ros2 launch robot_bringup rpi_stack.launch.py

# Terminal 3: WSL2 视觉
ros2 launch robot_bringup vision_stack.launch.py
```

详见 [QUICK_START.md](QUICK_START.md)

---

## 🔌 硬件接线

⚠️ **当前状态**：需要完成

**查看交互式接线图**：👉 [`hardware_wiring.html`](hardware_wiring.html)

**主要接线任务**：
1. STM32 ↔ 树莓派 UART (PA9/PA10 ↔ GPIO14/GPIO15)
2. 舵机 PWM (PA0~PA3) + 独立 5V 电源
3. 相机 USB (树莓派)
4. MPU6050 I2C (可选)

---

## 📊 项目进度

| 阶段 | 任务数 | 完成 | 状态 |
|---|---|---|---|
| 0 | 3 | 3 | ✅ |
| 1 | 5 | 5 | ✅ |
| 2 | 7 | 7 | ✅ |
| 3 | 7 | 7 | ✅ |
| 4 | 6 | 6 | ✅ |
| 5 | 3 | 3 | ✅ |
| 6 | 5 | 5 | ✅ |
| 7 | 6 | 2 | 🔄 (等待硬件) |
| **总计** | **40** | **35** | **87.5%** |

详见 [progress.md](progress.md)

---

## 🛠️ 技术栈

- **硬件**: STM32F103C8T6 + 树莓派 4B + NVIDIA GPU
- **框架**: ROS 2 Jazzy + FreeRTOS
- **AI**: YOLOv8n + ByteTrack + ONNX Runtime
- **通信**: ROS 2 DDS + UART + I2C

---

## ❓ 常见问题

**Q: 现在应该做什么？**  
A: 查看 [SETUP_GUIDE.md](SETUP_GUIDE.md) → 第 2 章硬件接线

**Q: 硬件接线完成后？**  
A: 查看 [QUICK_START.md](QUICK_START.md) 的启动流程

**Q: 如何调参 PID？**  
A: 见 [QUICK_START.md](QUICK_START.md) 的「PID 快速调参」

**Q: 系统架构是什么？**  
A: 见 [architecture.md](architecture.md)

---

## 📝 相关文档

- **设置指南**: [SETUP_GUIDE.md](SETUP_GUIDE.md)
- **快速开始**: [QUICK_START.md](QUICK_START.md)  
- **硬件接线**: [hardware_wiring.html](hardware_wiring.html)
- **系统架构**: [architecture.md](architecture.md)
- **任务清单**: [task.md](task.md)
- **项目进度**: [progress.md](progress.md)

---

**准备好了吗？** 👉 从 [SETUP_GUIDE.md](SETUP_GUIDE.md) 开始！
