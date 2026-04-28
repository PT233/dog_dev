# Dept.md

本文按 5 个模式整理当前仓库的技术债处理方案。范围只包含项目自有代码：

- `ros2_ws/src/**`
- `shared/**`
- `stm32_keil/Core/**`

不包含第三方 vendor 代码：

- `stm32_keil/Drivers/**`
- `stm32_keil/Middlewares/**`
- `ros2_ws/src/build/**`
- `ros2_ws/src/install/**`
- `ros2_ws/src/log/**`

---

## 模式 1：拆肥大的文件/函数

### 1.1 需要处理的文件

按当前代码行数统计，自有源码里真正超过 300 行的只有 1 个文件：

- `ros2_ws/src/uart_bridge/src/uart_bridge_node.cpp`：610 行

说明：

- `stm32_keil/Core/Src/system_stm32f1xx.c` 虽然 406 行，但属于 ST 生成的系统文件，不纳入本模式。
- 其他应用层文件都在 300 行以内，暂时不需要为了“达标”强拆。

### 1.2 拆分原则

- 不改函数内部逻辑，只搬位置和补 `#include`
- 对外头文件接口保持不变
- 新增的头文件只允许是内部实现头
- 拆完后每个 `.cpp` 文件不超过 300 行

### 1.3 建议拆分方案

建议把 `uart_bridge_node.cpp` 拆成 4 个实现文件 + 1 个内部头文件：

| 目标文件 | 预计行数 | 职责 |
| --- | ---: | --- |
| `uart_bridge/src/uart_bridge_time_sync.cpp` | ~180 | `TimestampMapper`、`LatencyMonitor`、`FrameSequenceChecker` |
| `uart_bridge/src/uart_bridge_transport.cpp` | ~170 | 构造/析构、串口打开配置、读线程、写串口 |
| `uart_bridge/src/uart_bridge_protocol.cpp` | ~220 | 握手、系统状态处理、帧解码、`/servo_cmd` 编码发送、统计输出 |
| `uart_bridge/src/uart_bridge_main.cpp` | <20 | `main()` |
| `uart_bridge/src/uart_bridge_node_internal.hpp` | ~120 | `UartBridgeNode` 私有声明，供以上 3 个 `.cpp` 共享 |

这样拆以后：

- 每个 `.cpp` 都低于 300 行
- 对外不新增公共 API
- `CMakeLists.txt` 只需要把原来的单文件可执行改成多文件可执行

### 1.4 拆分前后对照表

原文件：`ros2_ws/src/uart_bridge/src/uart_bridge_node.cpp`

| 原位置 | 原函数/类 | 拆分后位置 |
| --- | --- | --- |
| `:22` | `class TimestampMapper` | `uart_bridge_time_sync.cpp` |
| `:33` | `TimestampMapper::RecordMapping()` | `uart_bridge_time_sync.cpp` |
| `:41` | `TimestampMapper::MapTimestamp()` | `uart_bridge_time_sync.cpp` |
| `:74` | `class LatencyMonitor` | `uart_bridge_time_sync.cpp` |
| `:92` | `LatencyMonitor::RecordReceiveLatency()` | `uart_bridge_time_sync.cpp` |
| `:103` | `LatencyMonitor::GetStats()` | `uart_bridge_time_sync.cpp` |
| `:111` | `LatencyMonitor::ResetStats()` | `uart_bridge_time_sync.cpp` |
| `:123` | `class FrameSequenceChecker` | `uart_bridge_time_sync.cpp` |
| `:140` | `FrameSequenceChecker::CheckSequence()` | `uart_bridge_time_sync.cpp` |
| `:164` | `FrameSequenceChecker::GetTotalDropped()` | `uart_bridge_time_sync.cpp` |
| `:169` | `FrameSequenceChecker::ResetStats()` | `uart_bridge_time_sync.cpp` |
| `:178` | `class UartBridgeNode` 声明 | `uart_bridge_node_internal.hpp` |
| `:180` | `UartBridgeNode::UartBridgeNode()` | `uart_bridge_transport.cpp` |
| `:286` | `UartBridgeNode::~UartBridgeNode()` | `uart_bridge_transport.cpp` |
| `:315` | `UartBridgeNode::ReadLoop()` | `uart_bridge_transport.cpp` |
| `:335` | `UartBridgeNode::SystemStateToString()` | `uart_bridge_protocol.cpp` |
| `:350` | `UartBridgeNode::SendInitHandshake()` | `uart_bridge_protocol.cpp` |
| `:367` | `UartBridgeNode::OnSystemStateReceived()` | `uart_bridge_protocol.cpp` |
| `:404` | `UartBridgeNode::OnFrameReceived()` | `uart_bridge_protocol.cpp` |
| `:499` | `UartBridgeNode::OnServoCmdReceived()` | `uart_bridge_protocol.cpp` |
| `:556` | `UartBridgeNode::NameToServoId()` | `uart_bridge_transport.cpp` |
| `:564` | `UartBridgeNode::WriteFrame()` | `uart_bridge_transport.cpp` |
| `:586` | `UartBridgeNode::ReportStatistics()` | `uart_bridge_protocol.cpp` |
| `:604` | `main()` | `uart_bridge_main.cpp` |

### 1.5 不建议现在就拆的文件

- `ros2_ws/src/detection_node/src/yolo_infer.cpp`：224 行  
  还没超线，而且内部逻辑连续，强拆只会把预处理/后处理来回跳文件。
- `stm32_keil/Core/Src/main.c`：251 行  
  主要是 CubeMX 生成代码，应用逻辑占比很小。
- `stm32_keil/Core/Src/uart_rx_task.c`：244 行  
  目前仍然在“单一职责可读范围”内，先修共享状态和返回值问题更值。

---

## 模式 2：消除过度抽象

### 2.1 严格意义上的“只有一个实现的接口/抽象基类”

检索结果：

- 没有项目自定义的抽象基类
- 没有 `virtual ... = 0` 这类单实现接口
- 没有单例模式

结论：严格按“接口/抽象基类”定义，本项目这一类技术债为 `0`

### 2.2 单实现包装类/抽象层评估

虽然没有抽象基类，但有几层“只有一个具体实现的包装类”，值得顺手评估。

| 类 | 当前使用处 | 真实价值判断 | 是否建议内联化 | 去掉后的影响范围 |
| --- | --- | --- | --- | --- |
| `FrameEncoder` (`ros2_ws/src/uart_bridge/include/uart_bridge/frame_encoder.hpp:12`) | 生产代码 1 处：`uart_bridge_node.cpp:256`；测试 1 处：`tests/test_uart_frame_codec.cpp:67` | 有价值。它把“协议编码”从节点 I/O 里分离出来，测试也直接依赖它。 | 不建议 | 至少影响 2 个文件：`uart_bridge_node.cpp`、`tests/test_uart_frame_codec.cpp` |
| `FrameParser` (`ros2_ws/src/uart_bridge/include/uart_bridge/frame_parser.hpp:14`) | 生产代码 1 处：`uart_bridge_node.cpp:247`；测试 1 处：`tests/test_uart_frame_codec.cpp:77` 及后续多处 | 有价值。状态机和 CRC 校验从节点线程中分离是合理的。 | 不建议 | 至少影响 2 个文件：`uart_bridge_node.cpp`、`tests/test_uart_frame_codec.cpp` |
| `YoloInfer` (`ros2_ws/src/detection_node/include/detection_node/yolo_infer.hpp:33`) | 生产代码 1 处：`detection_node.cpp:30`；测试 1 处：`test_yolo_infer.cpp:13` | 有价值。它隔离了 ONNX Runtime/OpenCV 细节，节点本身只处理 ROS 输入输出。 | 不建议 | 至少影响 2 个文件：`detection_node.cpp`、`test_yolo_infer.cpp` |
| `PIDController` (`ros2_ws/src/visual_servo/include/visual_servo/pid_controller.hpp:12`) | 生产代码 1 处：`visual_servo_node.cpp:71-75`；测试 1 处：`tests/test_pid_controller.cpp` | 有价值。控制律单独成类便于测试和调参，不是空抽象。 | 不建议 | 至少影响 2 个文件：`visual_servo_node.cpp`、`tests/test_pid_controller.cpp` |
| `ByteTracker` (`ros2_ws/src/tracker_node/include/tracker_node/byte_tracker.hpp:43`) | 生产代码 1 处：`tracker_node.cpp:13`；测试 3 处：`tests/test_byte_tracker.cpp`、`tracker_node/test/test_byte_tracker.cpp`、`tracker_node/test/test_tracker_node_integration.cpp` | 有价值。跟踪算法与 ROS 节点边界清晰，测试覆盖也依赖它。 | 不建议 | 至少影响 4 个文件 |

### 2.3 模式 2 结论

- 本项目没有“为了抽象而抽象”的接口层
- 现有单实现包装类大多都承担了清晰的边界职责
- 模式 2 本轮不建议做代码删除式简化，收益不高，风险比模式 1/4/5 更差

---

## 模式 3：统一命名和风格

### 3.1 现状统计

统计口径：

- 类型名：`class` / `struct`
- 函数样式：按源码里出现的函数/方法标识符做唯一名统计
- 文件名：只看应用层自有文件

#### 类型名

基于 `ros2_ws/src`、`shared`、`stm32_keil/Core` 的类型定义：

- `PascalCase`：23
- 其他：5

结论：类型名已经高度统一，基本都是 `PascalCase`

#### 函数/方法名

按唯一标识符粗统计：

- `PascalCase`：108
- `snake_case`：70
- `camelCase`：16
- `UPPER_CASE`：76
- `other`：257

说明：

- `PascalCase` 主要来自项目自定义 C++ 方法：`OnTrackedObjects`、`LoadCocoClasses`、`ReportStatistics`
- `snake_case` 主要来自 C 函数、Python launch、topic/parameter 风格，以及少量 C++ 辅助函数：`crc16_ccitt`、`try_build_pipeline`、`image_callback`
- `camelCase` 大多是 FreeRTOS / CMSIS API：`xTaskCreate`、`osThreadNew`
- `UPPER_CASE` 大多是宏和测试宏，不属于业务命名层

#### 明显混用点

| 位置 | 现象 |
| --- | --- |
| `ros2_ws/src/behavior_node/src/behavior_node.cpp:48` | C++ 回调用 `OnTrackedObjects`，是 `PascalCase` |
| `ros2_ws/src/stereo_splitter/src/stereo_splitter_node.cpp:18` | 同类回调用 `image_callback`，是 `snake_case` |
| `ros2_ws/src/gst_receiver/src/gst_receiver_node.cpp:53` | 内部 helper 用 `try_build_pipeline`，是 `snake_case` |
| `shared/uart_protocol.c:3` | C 函数 `crc16_ccitt`，是 `snake_case` |
| `ros2_ws/src/visual_servo/include/visual_servo/visual_servo_node.hpp:20` | 类型已经重命名成 `LegMotionControllerNode` |
| `ros2_ws/src/visual_servo/src/visual_servo_node.cpp:1` | 文件名还保留 `visual_servo_node.cpp`，类名和文件语义脱节 |

### 3.2 推荐统一规范

结合 ROS 2 常见实践和当前仓库现状，建议采用下面这套规则：

#### C++ 代码

- 类型名：`PascalCase`  
  例如：`BehaviorNode`、`FrameParser`
- 函数/方法名：`snake_case`
  - 新代码统一改为 `snake_case`
  - 旧代码里的 `OnTrackedObjects`、`LoadCocoClasses`、`ReportStatistics` 逐步迁移
- 变量名：`snake_case`
- 成员变量：`snake_case_`
- `constexpr` 常量：`kPascalCase`
  例如：`kLegCount`
- 宏：`UPPER_CASE`

#### C 代码

- 函数名：`Module_Action` 或全 `snake_case` 二选一
- 当前仓库已经以 `snake_case` 为主，建议统一到 `snake_case`
  例如：`traj_set_target`、`status_tx_send_frame`
- 结构体字段：`snake_case`

#### 文件名

- 一律 `snake_case`
- 文件名与主类型/职责一致
  - 例如 `leg_motion_controller_node.cpp/.hpp`
  - 不要继续保留“旧文件名 + 新类名”的半重命名状态

### 3.3 迁移优先级

1. 先统一“文件名 vs 类型名”  
   典型案例：`visual_servo_node.*` 与 `LegMotionControllerNode`
2. 再统一 C++ 方法名风格  
   建议新改代码全部使用 `snake_case`
3. 最后处理老的 `PascalCase` 成员函数  
   这一步会影响引用较多，适合配合 `clang-tidy` 或批量重命名做

---

## 模式 4：消除重复代码

下面列 6 组“结构相同，只是变量名/常量不同”的近似重复代码。

### 4.1 UART 协议定义三份复制

重复位置：

- `shared/uart_protocol.h:1`
- `ros2_ws/src/uart_bridge/include/uart_bridge/uart_protocol.h:1`
- `stm32_keil/Core/Inc/uart_protocol.h:1`
- `shared/uart_protocol.c:1`
- `ros2_ws/src/uart_bridge/src/uart_protocol.c:1`
- `stm32_keil/Core/Src/uart_protocol.c:1`

建议抽象：

- 保留一份 canonical 版本在 `shared/uart_protocol.[ch]`
- ROS2 与 STM32 两端都直接包含/编译这份源文件
- 如果构建系统不方便共享路径，至少保留自动同步脚本，禁止手工改三份

嵌入式约束：

- 这是源码复用，不引入运行时开销

可读性变化：

- 明显提升，协议漂移风险大幅下降

### 4.2 ROS 单节点入口 `main()` 重复

重复位置：

- `behavior_node/src/main.cpp:4`
- `tracker_node/src/main.cpp:4`
- `gst_receiver/src/gst_receiver_main.cpp:3`
- `stereo_splitter/src/stereo_splitter_main.cpp:3`
- `detection_node/src/detection_viz_main.cpp:4`
- `detection_node/src/detection_node_main.cpp:3`
- `visual_servo/src/main.cpp:4`

建议抽象：

- 新增内部模板辅助：
  - `template<typename NodeT> int spin_single_node_main(int argc, char** argv)`
- 每个 `main.cpp` 只保留一行 `return spin_single_node_main<NodeT>(argc, argv);`

嵌入式约束：

- 仅限 ROS2 C++ 侧使用
- 模板内联，无虚函数开销

可读性变化：

- 提升明显，样板代码消失

### 4.3 COCO 类别文件加载重复

重复位置：

- `ros2_ws/src/behavior_node/src/behavior_node.cpp:109-132`
- `ros2_ws/src/detection_node/src/yolo_infer.cpp:14-25`

建议抽象：

- 提取一个共享辅助函数：
  - `std::vector<std::string> load_trimmed_lines(const std::string& path)`
- `BehaviorNode` 在返回的 `vector` 基础上构造 `id -> name` 映射
- `YoloInfer` 直接保存 `vector`

嵌入式约束：

- 仅 ROS2 侧使用，无 MCU 运行时压力

可读性变化：

- 中等提升
- 能把“文件读取”和“业务解释”分离

### 4.4 舵机 ID / 关节名映射重复

重复位置：

- `ros2_ws/src/uart_bridge/src/uart_bridge_node.cpp:448-451`
- `ros2_ws/src/uart_bridge/src/uart_bridge_node.cpp:482-485`
- `ros2_ws/src/uart_bridge/src/uart_bridge_node.cpp:556-561`
- `ros2_ws/src/visual_servo/src/visual_servo_node.cpp:11-16`
- `ros2_ws/src/visual_servo/src/visual_servo_node.cpp:18-25`
- `ros2_ws/src/visual_servo/src/visual_servo_node.cpp:189-193`

建议抽象：

- 提取 `constexpr std::array<const char*, 4> k_servo_names`
- 提供两个小 helper：
  - `const char* servo_id_to_name(uint8_t id)`
  - `int servo_name_to_id(std::string_view name)`

嵌入式约束：

- `constexpr + 小数组线性查找`
- 4 个元素，不值得为了这点映射引入哈希表或虚接口

可读性变化：

- 明显提升
- 关节命名不再散落在 3 个模块里

### 4.5 协议帧组装逻辑重复

重复位置：

- `ros2_ws/src/uart_bridge/src/frame_encoder.cpp:40-73`
- `stm32_keil/Core/Src/status_safety_task.c:85-117`
- `stm32_keil/Core/Src/status_safety_task.c:120-142`

建议抽象：

- 在 `shared/` 下新增一个零分配 C 帮助函数：
  - `size_t uart_build_frame(uint8_t cmd_id, const uint8_t* payload, size_t len, uint8_t* out, size_t cap)`
- STM32 端传入栈上 buffer
- ROS2 端 `FrameEncoder` 只做 `vector.resize + uart_build_frame(...)`

嵌入式约束：

- 不要用堆分配版本做 MCU 通用实现
- 先提供“调用者传 buffer”的 C API，再给 ROS2 端包一个轻量 C++ 外壳

可读性变化：

- 提升明显
- 现在相同的 header/len/crc/tail 拼接逻辑散在多个文件里

### 4.6 参数声明 + 读取样板重复

重复位置：

- `behavior_node.cpp:10-18`
- `detection_node.cpp:6-19`
- `tracker_node.cpp:8-10`
- `uart_bridge_node.cpp:186-192`
- `visual_servo_node.cpp:32-68`

建议抽象：

- 仅在 ROS2 C++ 层引入一个内联模板：
  - `declare_and_get<T>(node, name, default_value)`
- 或者每个节点定义自己的 `Params` 结构体和 `load_params(node)` 函数

嵌入式约束：

- 模板内联，无运行时分派成本

可读性变化：

- 小幅提升
- 但不要过度抽成“全项目统一参数框架”，否则会走到模式 2 的反面

---

## 模式 5：嵌入式特定问题修复

### 5.1 检查清单

| 检查项 | 结果 | 证据 | 结论 |
| --- | --- | --- | --- |
| ISR 里是否调用阻塞或非 ISR 安全 API | 基本通过 | `stm32_keil/Core/Src/stm32f1xx_it.c:211-227` 里只做标志位处理、`UartRxTask_NotifyFromIdleIrq()`、`HAL_UART_IRQHandler()`；`uart_rx_task.c:228-243` 使用的是 `vTaskNotifyGiveFromISR()` | 没看到用户代码在 ISR 里直接调用阻塞 API |
| FreeRTOS 栈大小是否做过水位评估 | 未通过 | `FreeRTOSConfig.h:99` 已开启 `INCLUDE_uxTaskGetStackHighWaterMark`，但项目里没有任何调用；任务创建点在 `freertos.c:99-105`、`uart_rx_task.c:219-224`、`traj_planner.c:145`、`status_safety_task.c:184-185` | 栈大小完全靠估计，没有实测依据 |
| 是否使用 `malloc/new` | 部分未通过 | `stm32_keil/Core` 里没有显式 `malloc/new`；但 `FreeRTOSConfig.h:60,107` 开启动态分配并选用 `heap_4`；`xQueueCreate()`、`xTaskCreate()`、`osThreadNew()` 会走堆分配 | 没有手写堆分配，但 RTOS 对象仍在运行时动态申请 |
| 共享变量是否有 `volatile` 或临界区保护 | 部分未通过 | 已标 `volatile`：`uart_rx_task.h:13-15`、`uart_rx_task.c:21`、`status_safety_task.c:30-31`；未保护共享状态：`traj_planner.c:24` 的 `g_traj_state[]` 被 `status_safety_task.c:104-105` 跨任务读取 | 共享状态保护不完整，`g_traj_state` 是当前最明显的问题 |
| 串口/SPI/I2C 是否用了 DMA + 中断 | 部分通过 | UART RX：`usart.c:91-104` 配 DMA，`uart_rx_task.c:184-191` 启 DMA + IDLE 中断，`stm32f1xx_it.c:220-223` 在 IDLE IRQ 里通知任务；UART TX：`status_safety_task.c:117,142` 仍然阻塞 `HAL_UART_Transmit()`；SPI/I2C 未见业务代码 | UART RX 路径是对的，TX 仍然是阻塞发送 |
| HAL 返回值是否检查 | 部分未通过 | 已检查：`usart.c:51-53,99-101,115-117`、`uart_rx_task.c:184-187`；未检查：`servo_driver.c:50` 的 `HAL_TIM_PWM_Start()`、`status_safety_task.c:117,142` 的 `HAL_UART_Transmit()`、`status_safety_task.c:168` 的 `HAL_IWDG_Refresh()` | 关键初始化有检查，但运行态 API 判错不完整 |
| ISR 里是否有浮点运算 | 通过 | `stm32f1xx_it.c` 的用户 ISR 路径未见浮点 | 不存在 M3/M4F ISR 浮点上下文问题 |

### 5.2 逐项说明

#### A. ISR 安全性

当前用户 ISR 路径相对克制：

- `USART1_IRQHandler()` 只清标志、发任务通知、再交给 HAL：`stm32f1xx_it.c:211-227`
- `UartRxTask_NotifyFromIdleIrq()` 用的是 ISR 安全 FreeRTOS API：`uart_rx_task.c:228-243`

这一项没有发现“在中断里 `HAL_Delay()` / 阻塞串口 / `osDelay()` / `xQueueSend()` 非 ISR 版本”的错误。

#### B. 栈大小

当前任务栈配置：

- `defaultTask`：`freertos.c:56-60`，`128 * 4`
- `Task_UART_RX`：`uart_rx_task.c:219-224`，`256`
- `TrajPlan`：`traj_planner.c:140-145`，`256U * 4U`
- `StatusTX`：`status_safety_task.c:174-178`，`256U * 4U`
- `Safety`：`status_safety_task.c:179-183`，`128U * 4U`

问题不在“数字一定错”，而在：

- 配置单位混着来：`xTaskCreate()` 的 `256` 是 stack depth，`osThreadNew()` 的 `256U * 4U` 是字节
- 工程虽然启用了 `uxTaskGetStackHighWaterMark`，但没有任何采样代码

建议：

- 在 bring-up 阶段给每个任务打一次高水位日志
- 把“保守估算”改成“实测 + 留 25% 余量”

#### C. 动态内存

严格按“裸机/MCU 上应避免动态内存”这一标准，当前状态不够好：

- 没有手写 `malloc/new`
- 但 `xQueueCreate()`、`xTaskCreate()`、`osThreadNew()` 都会用到 `heap_4`

如果后续要把系统做成长期运行的固件，建议：

1. 评估改成静态任务/静态队列 API
2. 至少把“启动时一次性动态分配”与“运行期动态分配”区分开
3. 当前默认空转的 `defaultTask` 可以先删掉，减少堆与栈占用

#### D. 共享变量保护

当前最值得先修的是 `g_traj_state`：

- 写入点：`traj_planner.c:45-73`, `traj_planner.c:77-125`
- 读取点：`status_safety_task.c:101-108`

问题：

- 一个任务更新轨迹状态
- 另一个任务异步打包状态帧
- 中间没有临界区、没有快照复制、也没有双缓冲

这会导致：

- `current_angle` 与 `duration_ms` 可能来自不同步时刻
- 读到“半更新”的组合状态

建议：

- 轻量方案：在 `StatusTX_SendFrame()` 里先拷贝一份局部快照，再编码发送
- 更稳方案：`taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` 包住 4 路状态复制

#### E. DMA / 中断 vs 阻塞调用

当前串口链路是“半套异步”：

- RX 很合理：DMA circular + IDLE IRQ + 任务处理
- TX 配了 DMA 通道：`usart.c:106-120`
- 但实际发送还是阻塞 `HAL_UART_Transmit()`：`status_safety_task.c:117,142`

这意味着：

- 配置了 TX DMA，但没有真正吃到收益
- 状态上报任务仍然会被 UART 发送时间绑住

建议：

- 如果状态上报频率继续维持 20Hz，可以先把返回值检查补齐
- 如果后续上报频率提高或 payload 变大，再把 TX 切到 DMA

#### F. HAL 返回值检查

当前问题点：

- `servo_driver.c:50`  
  `HAL_TIM_PWM_Start()` 没判错
- `status_safety_task.c:117,142`  
  `HAL_UART_Transmit()` 没判错
- `status_safety_task.c:168`  
  `HAL_IWDG_Refresh()` 没判错

建议优先顺序：

1. 先补 `HAL_UART_Transmit()` 和 `HAL_TIM_PWM_Start()`
2. 再给关键 RTOS 创建返回值加断言/日志
3. 最后考虑统一错误上报码路径

### 5.3 模式 5 的优先修复项

按风险排序，建议先做这 4 件事：

1. 给 `g_traj_state` 增加读取快照或临界区保护
2. 给 `HAL_UART_Transmit()` / `HAL_TIM_PWM_Start()` 补返回值检查
3. 补充 `uxTaskGetStackHighWaterMark()` 的实测日志
4. 评估把 `defaultTask` 删掉，并逐步改成静态 RTOS 对象

---

## 总结

按这 5 个模式看，当前项目最值得先做的是：

1. 模式 1：拆 `uart_bridge_node.cpp`
2. 模式 4：收敛协议定义、关节映射、ROS2 `main()` 样板
3. 模式 5：补 STM32 侧共享状态保护、判错和栈水位观测

模式 2 不是当前主矛盾；模式 3 则适合配合后续重构顺手统一。
