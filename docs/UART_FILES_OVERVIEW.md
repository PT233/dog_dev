# UART 通信涉及的所有文件总览

## 📁 文件清单（按目录组织）

### 1️⃣ 共享协议层 (`shared/`)

| 文件 | 行数 | 功能 |
|-----|------|------|
| `uart_protocol.h` | 42 | 协议定义、结构体、CRC 原型 |
| `uart_protocol.c` | 27 | CRC16-CCITT 算法实现 |

**关键内容**：
- 帧格式常量：`UART_FRAME_HEADER_0/1`, `UART_FRAME_TAIL`
- 命令类型：`UART_CMD_SERVO_CONTROL` (0x01), `UART_CMD_SERVO_STATE` (0x81)
- 结构体：`ServoCmdItem`, `ServoStateItem`
- 函数：`crc16_ccitt()`

---

### 2️⃣ STM32 固件 (`stm32_fw/Core/`)

#### 驱动配置

| 文件 | 行数 | 功能 |
|-----|------|------|
| `Src/usart.c` | 167 | UART1/DMA 初始化配置 |
| `Inc/usart.h` | ~50 | UART 头文件声明 |

**关键配置**：
- UART1: 921600 bps, 8N1, DMA 循环接收
- PA9 (TX): 复用推挽输出
- PA10 (RX): 复用输入 + 上拉
- DMA1_Channel5 (RX): 循环模式
- DMA1_Channel4 (TX): 单次模式
- IDLE 中断使能

#### UART 应用层

| 文件 | 行数 | 功能 |
|-----|------|------|
| `Src/uart_rx_task.c` | 213 | UART 接收任务、帧解析、CRC 校验 |
| `Inc/uart_rx_task.h` | ~40 | 接收任务声明、队列导出 |

**关键函数**：
- `UartRxTask_Create()` - 创建接收任务
- `UartRxTask_NotifyFromIdleIrq()` - IDLE 中断回调
- `UartRx_ParseByte()` - 8 态状态机
- `UartRx_HandleFrame()` - CRC 校验、命令派发

#### 中断处理（自动生成）

| 文件 | 行数 | 功能 |
|-----|------|------|
| `Src/stm32f1xx_it.c` | ~500 | 全局中断处理 |
| `Inc/stm32f1xx_it.h` | ~50 | 中断声明 |

**关键处理**：
- `USART1_IRQHandler()` - UART 中断入口
- 调用 `UartRxTask_NotifyFromIdleIrq()`
- 调用 `HAL_UART_IRQHandler()`

#### 链接脚本 & Makefile

| 文件 | 功能 |
|-----|------|
| `STM32F103XX_FLASH.ld` | 链接脚本 |
| `Makefile` | 编译配置 |

---

### 3️⃣ 树莓派 ROS 2 节点 (`ros2_ws/src/uart_bridge/`)

#### 协议实现（本地副本）

| 文件 | 行数 | 功能 |
|-----|------|------|
| `src/uart_protocol.c` | 27 | CRC16-CCITT（副本） |
| `include/uart_bridge/uart_protocol.h` | 42 | 协议定义（副本） |

**说明**：与 shared/ 中的文件相同，复制到包内

#### 帧解析器

| 文件 | 行数 | 功能 |
|-----|------|------|
| `src/frame_parser.cpp` | 104 | 8 态状态机、CRC 验证 |
| `include/uart_bridge/frame_parser.hpp` | 50 | 帧解析器类声明 |

**关键类**：`FrameParser`
- 状态：8 个（WAIT_HEADER_0/1, READ_CMD_ID, READ_LEN, READ_PAYLOAD, READ_CRC_0/1, READ_TAIL）
- 回调：`FrameCallback`, `ErrorCallback`
- 方法：`ProcessByte()`, `SetFrameCallback()`, `SetErrorCallback()`

#### 帧编码器

| 文件 | 行数 | 功能 |
|-----|------|------|
| `src/frame_encoder.cpp` | 63 | 帧格式化、CRC 计算 |
| `include/uart_bridge/frame_encoder.hpp` | 26 | 帧编码器类声明 |

**关键类**：`FrameEncoder`
- 方法：`EncodeServoControl()`, `EncodeSingleServo()`, `BuildFrame()`

#### 节点主体

| 文件 | 行数 | 功能 |
|-----|------|------|
| `src/uart_bridge_node.cpp` | 243 | UART 设备、ROS 2 集成 |
| `include/uart_bridge/uart_bridge.hpp` | ~80 | 节点类声明 |

**关键类**：`UartBridgeNode`
- 构造函数：UART 设备打开、termios 配置
- 方法：`ReadLoop()`, `OnFrameReceived()`, `OnServoCmdReceived()`, `WriteFrame()`
- 订阅：`/servo_cmd` (JointState)
- 发布：`/servo_state` (JointState)

#### 编译配置

| 文件 | 功能 |
|-----|------|
| `CMakeLists.txt` | ROS 2 编译配置 |
| `package.xml` | ROS 2 包描述 |
| `config/uart_bridge.yaml` | 参数配置（波特率、设备路径） |

---

### 4️⃣ 单元测试 (`tests/`)

#### CRC 测试

| 文件 | 行数 | 功能 |
|-----|------|------|
| `test_uart_protocol/test_crc.c` | ~150 | CRC16 算法验证 |

**测试用例**：5 个
- 空输入
- 单字节 0x00
- 标准向量："123456789" → 0x29B1
- 随机 16 字节
- 协议完整帧

#### 帧解析测试

| 文件 | 行数 | 功能 |
|-----|------|------|
| `test_uart_frame_parser.cpp` | ~260 | 树莓派端帧解析验证 |

**测试用例**：4 个
- 解析舵机状态帧（0x81）
- 解析舵机控制帧（0x01）
- 拒绝 CRC 错误帧
- 从垃圾字节恢复

#### 帧编码测试

| 文件 | 行数 | 功能 |
|-----|------|------|
| `test_uart_encode_send.cpp` | ~312 | 树莓派端帧编码验证 |

**测试用例**：4 个
- 编码单个舵机命令
- 编码多个舵机命令
- 角度转换验证（0°, 45°, 90°, 135°, 180°）
- Servo ID 映射验证

---

### 5️⃣ 文档 (根目录)

| 文件 | 功能 |
|-----|------|
| `UART_VERIFICATION_REPORT.md` | 详细验收报告（8 章） |
| `UART_VERIFICATION_SUMMARY.txt` | 执行总结 |
| `UART_FINAL_CHECKLIST.md` | 验收清单（138 项） |

---

## 📊 文件统计

### 代码行数统计

```
┌─ 共享协议层
│  ├─ uart_protocol.h      42 行
│  └─ uart_protocol.c      27 行
│                          ────
│                           69 行
│
├─ STM32 固件
│  ├─ usart.c             167 行
│  ├─ usart.h              50 行
│  ├─ uart_rx_task.c      213 行
│  ├─ uart_rx_task.h       40 行
│  ├─ stm32f1xx_it.c      500 行
│  └─ stm32f1xx_it.h       50 行
│                          ────
│                         1020 行
│
├─ 树莓派 ROS 2
│  ├─ uart_protocol.c      27 行 (副本)
│  ├─ uart_protocol.h      42 行 (副本)
│  ├─ frame_parser.cpp    104 行
│  ├─ frame_parser.hpp     50 行
│  ├─ frame_encoder.cpp    63 行
│  ├─ frame_encoder.hpp    26 行
│  ├─ uart_bridge_node.cpp 243 行
│  ├─ uart_bridge.hpp      80 行
│  ├─ CMakeLists.txt       50 行
│  ├─ package.xml          30 行
│  └─ config/uart_bridge.yaml  10 行
│                          ────
│                          725 行
│
├─ 单元测试
│  ├─ test_crc.c          150 行
│  ├─ test_frame_parser   260 行
│  └─ test_encode_send    312 行
│                          ────
│                          722 行
│
└─ 文档
   ├─ UART_VERIFICATION_REPORT.md
   ├─ UART_VERIFICATION_SUMMARY.txt
   └─ UART_FINAL_CHECKLIST.md

总计：约 2536 行生产代码 + 722 行测试代码 + 文档
```

---

## 🔗 文件依赖关系图

```
┌─────────────────────────────────────────────────────────────┐
│ shared/uart_protocol.h/c                                    │
│ (公共协议、帧格式、CRC 算法)                                  │
└──────────────┬────────────────────────────────────┬──────────┘
               │                                    │
        ┌──────▼──────┐                    ┌────────▼────────┐
        │ STM32 固件   │                    │ 树莓派 ROS 2    │
        │ (C/FreeRTOS) │                    │ (C++/ROS2)      │
        ├──────────────┤                    ├─────────────────┤
        │ usart.c      │ ─DMA/UART─────── │ uart_bridge_   │
        │ (硬件配置)   │   IDLE 中断       │ node.cpp       │
        ├──────────────┤                    │ (UART 设备)    │
        │ uart_rx_     │                    ├─────────────────┤
        │ task.c       │◄─────────────────►│ frame_parser.cpp│
        │ (帧解析、    │  FreeRTOS 队列    │ (帧解析)        │
        │  CRC 校验)   │  + 任务通知       ├─────────────────┤
        │              │                   │ frame_encoder   │
        │ stm32f1xx_   │                   │ .cpp            │
        │ it.c         │                   │ (帧编码)        │
        │ (中断处理)   │                   └─────────────────┘
        │              │                            △
        │ servo_driver │                            │ ROS 2 话题
        │ traj_planner │                    /servo_cmd (下发)
        │ (业务逻辑)   │                    /servo_state (反馈)
        └──────┬───────┘                            │
               │ PWM 50Hz                           │
               ▼                            ┌────────▼────────┐
         SG90 舵机 ×4                       │ detection_node  │
                                            │ behavior_node   │
                                            │ visual_servo    │
                                            │ (WSL2 应用)     │
                                            └─────────────────┘
```

---

## 🔍 按功能分类的文件关系

### 帧接收路径

```
stm32_fw/Core/Src/stm32f1xx_it.c (USART1_IRQHandler)
    ↓
stm32_fw/Core/Src/uart_rx_task.c (UartRxTask_NotifyFromIdleIrq)
    ↓
stm32_fw/Core/Src/uart_rx_task.c (Task_UART_RX)
    ↓
stm32_fw/Core/Src/uart_rx_task.c (UartRx_ParseByte - 状态机)
    ↓
shared/uart_protocol.c (crc16_ccitt)
    ↓
stm32_fw/Core/Src/uart_rx_task.c (UartRx_HandleFrame - CRC 校验)
    ↓
stm32_fw/Core/Src/uart_rx_task.c (xQueueSend)
```

### 帧发送路径

```
ros2_ws/src/uart_bridge/src/uart_bridge_node.cpp (OnServoCmdReceived)
    ↓
ros2_ws/src/uart_bridge/src/frame_encoder.cpp (EncodeServoControl)
    ↓
shared/uart_protocol.c (crc16_ccitt - 副本在 uart_bridge 包内)
    ↓
ros2_ws/src/uart_bridge/src/uart_bridge_node.cpp (WriteFrame)
    ↓
/dev/ttyAMA0 (UART 设备)
```

### 帧接收反馈路径

```
/dev/ttyAMA0 (UART 设备)
    ↓
ros2_ws/src/uart_bridge/src/uart_bridge_node.cpp (ReadLoop)
    ↓
ros2_ws/src/uart_bridge/src/frame_parser.cpp (ProcessByte - 状态机)
    ↓
ros2_ws/src/uart_bridge/src/uart_protocol.c (crc16_ccitt - 副本)
    ↓
ros2_ws/src/uart_bridge/src/uart_bridge_node.cpp (OnFrameReceived)
    ↓
ROS 2 /servo_state 话题发布
```

---

## 📋 核心文件清单（必须存在）

### 🔴 生产代码（必须）

- ✅ `shared/uart_protocol.h` - 协议定义
- ✅ `shared/uart_protocol.c` - CRC 实现
- ✅ `stm32_fw/Core/Src/usart.c` - UART 硬件配置
- ✅ `stm32_fw/Core/Src/uart_rx_task.c` - 接收任务
- ✅ `stm32_fw/Core/Src/stm32f1xx_it.c` - 中断处理
- ✅ `ros2_ws/src/uart_bridge/src/uart_bridge_node.cpp` - 节点主体
- ✅ `ros2_ws/src/uart_bridge/src/frame_parser.cpp` - 帧解析
- ✅ `ros2_ws/src/uart_bridge/src/frame_encoder.cpp` - 帧编码

### 🟡 配置文件（必须）

- ✅ `ros2_ws/src/uart_bridge/package.xml` - ROS 2 包配置
- ✅ `ros2_ws/src/uart_bridge/CMakeLists.txt` - 编译配置
- ✅ `ros2_ws/src/uart_bridge/config/uart_bridge.yaml` - 运行时参数

### 🟢 测试文件（可选但推荐）

- ✅ `tests/test_uart_protocol/test_crc.c`
- ✅ `tests/test_uart_frame_parser.cpp`
- ✅ `tests/test_uart_encode_send.cpp`

### 🔵 文档（参考）

- ✅ `UART_VERIFICATION_REPORT.md`
- ✅ `UART_VERIFICATION_SUMMARY.txt`
- ✅ `UART_FINAL_CHECKLIST.md`

---

**总计**：
- 📄 **20+ 核心源文件**
- 🔗 **2536 行生产代码**
- 🧪 **722 行测试代码**

