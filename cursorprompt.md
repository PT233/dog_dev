# 嵌入式 C/C++ 工程 Vibe Coding 工作流

## 工具选择

- **架构 / 规划阶段**：长上下文对话型 LLM（Claude Opus 最稳，因为它在硬件约束推理和不瞎编 HAL 上表现较好；Gemini 2.5 Pro 也行）。
- **执行阶段**：Claude Code 或 Cursor。Aider 在嵌入式项目里也很顺手，因为它对 git diff 友好，方便你逐个 commit 审。
- **额外建议**：把芯片的 datasheet、Reference Manual、HAL 用户手册的关键章节（中断、时钟树、目标外设）放到项目根目录的 `docs/refs/` 下，并在系统提示里要求 LLM 优先查这些文件。

---

## 步骤 0：硬件与平台约束清单（新增，非常重要）

在写 architecture 之前，先建一个 `hardware.md`，让 LLM 在所有后续步骤里都能读到。模板：

```markdown
# Hardware & Platform Constraints

## MCU
- 型号：STM32F407VGT6 / ESP32-S3 / nRF52840 / ...
- 主频：168 MHz
- Flash：1 MB
- RAM：192 KB（含 64 KB CCM）
- FPU：单精度硬件 FPU

## 工具链
- 编译器：arm-none-eabi-gcc 13.2
- 构建系统：CMake + Ninja / PlatformIO / STM32CubeIDE
- 调试器：ST-Link V3 / J-Link
- RTOS：FreeRTOS 10.x / 裸机 / Zephyr
- HAL：STM32 HAL / LL / ESP-IDF / nRF SDK / 自写

## 外设使用计划
| 外设 | 用途 | 引脚 | 备注 |
|------|------|------|------|
| USART1 | 调试日志 | PA9/PA10 | 115200 8N1 |
| SPI1 | 外部 Flash | PA5-7, PA4(CS) | 25 MHz |
| TIM2 | PWM 输出 | PA0 | 20 kHz |
| ... |

## 资源预算（硬约束）
- Flash 占用 ≤ 70%（留 30% OTA / 调试）
- 静态 RAM ≤ 50%
- 最大栈深度 ≤ 4 KB（每任务）
- 任意中断 ISR ≤ 50 μs
- 主循环周期 ≤ 1 ms

## 不允许使用
- 动态内存分配（malloc/new）—— 全部用静态分配 / 内存池
- C++ 异常、RTTI
- 标准库流（std::cout, iostream）
- printf 浮点格式（占 Flash 太大），用整数格式 + 定点
```

> 这个文件是后续所有 prompt 的"宪法"。每次给 LLM 任务都要求它先读这个文件。

---

## 步骤 1：生成 architecture.md

> 我要在 [MCU 型号] 上构建 [产品]。
>
> **产品描述**：[一句话]
> **核心功能**（按优先级）：
>   1. ...
>   2. ...
> **实时性要求**：[例如 "电机控制 PWM 必须 20 kHz 抖动 < 1%"]
> **接口**：[USB CDC / BLE / 以太网 / Modbus / ...]
>
> 项目根目录已有 `hardware.md`，请先读完，再生成 `architecture.md`，必须包含：
>
> 1. **分层架构图**（文字描述）：
>    - HAL 层（厂商提供）
>    - 驱动层（你写的外设封装）
>    - 服务层（业务逻辑、状态机）
>    - 应用层（任务调度、main）
>    - 明确每层的依赖方向（只能向下依赖）
>
> 2. **任务 / 中断划分表**：
>    | 名称 | 类型（Task/ISR/裸循环） | 优先级 | 周期或触发源 | 栈大小 | 主要职责 |
>
> 3. **数据流与共享状态**：
>    - 哪些数据跨任务共享
>    - 用什么同步机制（队列 / 互斥锁 / 原子变量 / 双缓冲 / lock-free 环形缓冲）
>    - 明确 ISR 与任务之间的通信方式（队列 from ISR / task notification）
>
> 4. **时钟树与外设初始化顺序**
>
> 5. **错误处理策略**：
>    - 致命错误：复位 / 进入 safe mode / 记录到 NVM
>    - 可恢复错误：重试策略
>    - 看门狗策略（独立看门狗 + 窗口看门狗 + 软件看门狗）
>
> 6. **资源预算分配**：把 `hardware.md` 里的总预算分到各模块（Flash / RAM / 栈 / CPU%）
>
> 7. **关键设计决策**（至少 3 条），每条说明：
>    - 选了什么
>    - 为什么不选替代方案
>    - 资源代价（Flash / RAM / CPU）
>
> 8. **MVP 不做的事**（明确边界，比如"暂不支持 OTA"、"暂不做低功耗"）
>
> **要求**：
> - 全程引用 `hardware.md` 里的具体数字
> - 任何 HAL 函数 / 寄存器名都必须给出在 Reference Manual 的章节号；不确定就写 "TBD - 需查 RM 第 X 节"，绝对不准编
> - 涉及 RTOS API 时，明确指明用哪个版本（FreeRTOS / CMSIS-RTOS v2）
> - 遇到我没说清楚的硬件细节（外设连法、传感器型号、电源域），先列出假设并停下来问

---

## 步骤 2：生成 task.md

> 基于 `architecture.md` 和 `hardware.md`，生成 `task.md`。
>
> **嵌入式特有的任务粒度要求**：
> - 每个任务改动 ≤ 3 个 .c/.h 对
> - 每个任务必须有"在硬件上的可观测验证方式"（LED 闪、串口打印、示波器波形、逻辑分析仪信号等）—— 不能只靠"代码编译过"
> - 每个任务必须给出 Flash / RAM 增量预估（即使是粗估，比如 "+~500 B Flash, +~32 B BSS"）
> - 涉及中断、时钟、电源的任务单独标记为 ⚠️ HIGH RISK
>
> **任务排序原则**（嵌入式专用）：
> 1. **Bring-up 优先**：时钟配置 → 调试串口 → LED 心跳。这三个不通别的全免谈。
> 2. **每个外设单独 bring-up 任务**：先用最简单的方式驱动起来（比如 SPI 先收发 0xAA 自环），再上业务逻辑
> 3. **中断与 DMA 后置**：先用阻塞 / 轮询版本验证逻辑正确，再改成中断 / DMA
> 4. **RTOS 任务集成放最后**：先各模块独立测，再集成到调度器里
> 5. **看门狗最后才开**：调试期先关掉，集成测试阶段才打开
>
> **每个任务必须包含**：
> - 编号（T01...）
> - 标题
> - 前置依赖
> - 涉及文件
> - **硬件验证方式**（具体到"用万用表测 PA5"或"示波器看 TIM2_CH1，应见 20kHz 50% 占空比"）
> - **回归风险**：这次改动可能影响的其他模块
> - 预估资源增量（Flash / RAM）
> - ⚠️ HIGH RISK 标记（如适用）

---

## 步骤 3：生成 test.md

> 基于 `task.md` 生成 `test.md`，分四层：
>
> 1. **Host 单元测试**（PC 上跑，用 Unity / Ceedling / GoogleTest）：
>    - 哪些模块可以脱离硬件测试（纯算法、状态机、协议解析、数据结构）
>    - 对每个：列出测试用例（正常 / 边界 / 异常 / 字节序 / 溢出）
>    - 用什么方式 mock HAL（建议：把 HAL 调用包一层薄 wrapper，单测时 link mock 实现）
>
> 2. **Target 模块测试**（在板子上单模块跑）：
>    - 每个驱动写一个独立 demo main，验证基本功能
>    - 明确每个 demo 的"通过判据"（串口输出什么、波形长什么样）
>
> 3. **集成测试**（板子上完整系统）：
>    - 端到端场景清单
>    - 长时间稳定性测试（建议至少 24 小时）
>    - 异常注入测试（拔传感器、断电重启、通信线短路、电压波动）
>
> 4. **资源审计**：
>    - Flash / RAM 占用检查脚本（解析 .map 文件）
>    - 栈水位检查（FreeRTOS uxTaskGetStackHighWaterMark / 填充 0xA5 法）
>    - CPU 占用检查（GPIO 翻转 + 逻辑分析仪 / runtime stats）
>    - 中断延迟测量方法
>
> 每条测试关联到 `task.md` 的任务编号。

---

## 步骤 4：交给编码 agent 执行

`progress.md` 初始结构：

```markdown
# Progress

## 已完成
（空）

## 进行中
（空）

## 待办
见 task.md

## 阻塞 / 待人工决策
（空）

## 资源水位（每完成一个任务更新）
| 时间点 | 任务 | Flash 用量 | RAM 用量 | 备注 |
|--------|------|-----------|----------|------|
```

给 agent 的提示词：

> 你是这个嵌入式项目的工程师。项目根目录有：
> - `hardware.md`：硬件平台与资源约束（**这是最高优先级，所有决策必须符合**）
> - `architecture.md`：分层架构与任务划分
> - `task.md`：任务清单
> - `test.md`：测试计划
> - `progress.md`：进度（你要持续更新）
> - `docs/refs/`：芯片 datasheet / RM / HAL 文档
>
> **每个任务的工作流程**：
> 1. 读 `progress.md`，确认下一个任务编号
> 2. 读对应任务，特别注意是否有 ⚠️ HIGH RISK 标记
> 3. **先用一段话告诉我**：
>    - 你打算改哪些文件
>    - 涉及哪些 HAL 函数 / 寄存器（给出 RM 章节号）
>    - 资源增量预估
>    - 主要风险点
>    然后等我确认
> 4. 实施改动
> 5. 跑 host 单元测试（如果该任务有）
> 6. 编译，输出 size 命令的结果，更新 `progress.md` 的资源水位表
> 7. 告诉我硬件上怎么验证（具体到测哪个引脚 / 看什么波形 / 串口预期输出）
> 8. 等我回报硬件验证结果
> 9. 通过 → 追加到"已完成"区，停下来等"继续"
>
> **嵌入式开发守则（EMBEDDED CODING PROTOCOL）**：
>
> **代码风格**
> 1. 严格 C99 或 C++17，不用编译器扩展（除非 `hardware.md` 明确允许）
> 2. 头文件用 `#pragma once` 或传统 include guard，二选一全项目统一
> 3. 所有公共函数加 doxygen 风格注释（@brief / @param / @return），用英文
> 4. 私有函数加 `static`，私有变量也是
> 5. 模块命名前缀（如 `motor_`, `comm_`），避免符号污染
>
> **资源约束**
> 6. **绝不调用** malloc/new/free/delete（除非任务明确批准）
> 7. 大对象（>64B）必须静态分配或来自内存池
> 8. 不要在 ISR 里做：浮点运算、调用阻塞函数、长循环、申请互斥锁
> 9. 不要用 `printf` 打印浮点（用定点 + 手动格式化）；调试日志用专用 logger 模块
> 10. 字符串字面量超过 32 字符的考虑放 Flash（const）
>
> **类型与安全**
> 11. 整数用 `<stdint.h>` 的 `uint8_t / int32_t` 等，禁用裸 `int / long`（位宽不确定）
> 12. 寄存器访问必须 `volatile`
> 13. 跨字节序数据（网络包、外部 Flash）显式标注大小端
> 14. 位域慎用（编译器实现不一致），优先用 `(reg & MASK) >> SHIFT`
> 15. 所有 HAL 函数返回值必须检查（HAL_OK / 错误码）
>
> **并发与中断**
> 16. ISR 与任务共享变量必须 `volatile`，且要么是原子类型，要么用临界区
> 17. 临界区必须最短（measure if needed），不在临界区内做 I/O 或长计算
> 18. 用 RTOS 时：队列优于全局变量，notification 优于信号量（更省 RAM）
>
> **C++ 特有**（如果项目用 C++）
> 19. 不用异常、RTTI、`std::string`、`std::vector`（除非有 PMR allocator）
> 20. 类成员初始化用初始化列表
> 21. 默认 `constexpr` / `noexcept` 能加就加
> 22. 单实例的"驱动类"用静态成员 + 私有构造，不要用单例模板
> 23. 模板优于虚函数（只要不爆 Flash）；虚函数只在确实需要运行时多态时用
>
> **硬件交互**
> 24. **不准编造寄存器名、位定义、HAL 函数签名**。不确定就停下来问，或在注释里写 `// FIXME: 待查 RM 第 X.Y 节`
> 25. 写新驱动前，先在 `progress.md` 的"待人工决策"区列出：用哪个外设、哪个时钟源、引脚复用配置；等我确认
> 26. 涉及时钟、电源、看门狗的改动一律先停下来问
>
> **构建与配置**
> 27. 修改 CMakeLists / Makefile / linker script 前先告诉我
> 28. 加新依赖（第三方库）必须先经我同意，并说明它的 Flash / RAM 占用
> 29. 编译选项不擅自动（特别是 `-O` 等级、`-flto`、`-fexceptions`）
>
> **不确定就停**
> 30. 任何超出当前任务范围、涉及硬件配置变更、或与 `hardware.md` 冲突的情况，**停下来问**，写入 `progress.md` 的"阻塞"区
> 31. 测试失败时不要反复试错凑过去；先在 `progress.md` 写下"现象 + 怀疑方向 + 建议下一步"，等我介入

---

## 步骤 5：人工节奏

每个任务完成后：
1. 看 `progress.md` 的改动摘要 + 资源水位
2. 编译并烧录
3. 按 agent 给的硬件验证方法实测（万用表 / 示波器 / 串口）
4. 通过 → `git commit`，回 "继续"
5. 不通过 → 把现象（带数据，比如"示波器频率 19.8 kHz，预期 20 kHz"）贴回去
6. **每完成 5 个任务做一次回归**：跑完整 host 单元测试 + 长时间运行（至少 1 小时）

---

# 附录 A：嵌入式技术债盘点 prompt

> 审查整个嵌入式项目，输出技术债清单。**嵌入式专用关注点**：
>
> **资源浪费**
> - Flash 占用大户（用 `arm-none-eabi-nm --size-sort` 或 map 文件分析）
> - 大块 BSS / DATA（>1 KB 的全局数组）
> - 字符串字面量浪费（重复字符串、长 debug 字符串）
> - 未使用 `const` 导致变量进 RAM 而非 Flash
>
> **实时性风险**
> - 长 ISR（>50 行 / 调用了非 ISR-safe 的 HAL 函数）
> - ISR 里的浮点运算 / 除法 / 长循环
> - 任务里的 busy-wait（应该用阻塞 API 或事件）
> - 临界区过长
>
> **可靠性问题**
> - HAL 函数返回值未检查
> - 缓冲区溢出风险（strcpy / sprintf / 无边界检查的拷贝）
> - 整数溢出 / 隐式类型转换（特别是 `int` ↔ `uint`）
> - 未初始化变量（特别是 struct）
> - 共享变量没 `volatile` 或没保护
>
> **可移植性问题**
> - 假设字节序 / 对齐 / 指针大小
> - 硬编码寄存器地址（应该用 CMSIS 头文件）
> - 编译器特定语法（`__attribute__` 等）未做兼容封装
>
> **嵌入式过度设计**
> - 用了虚函数但只有一个实现（白白吃 Flash + RAM 的 vtable）
> - 用了 `std::function` / lambda 捕获（堆分配风险）
> - 单例 / 工厂模式在只有一个实例的场景下使用
> - 抽象出来的 "platform layer" 实际只支持一个平台
>
> 每条给出：`文件:行号` / 严重程度（高/中/低）/ 修复成本 / 资源收益（如 "省 ~200B Flash"）。

---

# 附录 B：嵌入式专用重构模板

**模板 1 — 拆肥大的驱动文件**
> `xxx_driver.c` 太大。按职责拆成：寄存器层 / 协议层 / 业务层。要求：
> - 寄存器层只做读写，不带逻辑
> - 协议层不直接碰寄存器
> - 对外 API（在 .h 里的）保持不变
> - 拆完跑 size 命令，确认 Flash / RAM 没增加（理想是减少）

**模板 2 — 干掉假抽象**
> 列出项目里"只有一个实现的接口 / 虚基类 / 函数指针表"。对每个判断：
> - 真的需要运行时多态吗（未来会换实现吗）？
> - 不需要 → 内联成具体调用，省掉 vtable / 函数指针的 Flash + RAM 开销
> - 给出预估收益（每个虚函数 ~4B Flash + 每个对象 4B RAM）

**模板 3 — 替换 malloc**
> 找出所有 malloc/new 调用点。对每个：
> - 分析最大 / 最常用 size
> - 给出替代方案：静态分配 / 固定大小池 / 栈分配 / placement new
> - 评估改动后的 worst-case RAM 用量

**模板 4 — ISR 瘦身**
> 找出执行时间可能 >50 μs 的 ISR。对每个：
> - 把可延后的工作挪到任务（task notification / 队列）
> - 把浮点 / 除法挪走
> - 给出改造前后的预估执行时间

**模板 5 — printf 替换**
> 找出所有 `printf / sprintf / snprintf` 调用。评估：
> - 是否使用了浮点格式（`%f`）→ 替换为定点 + 手动格式化
> - 是否在 ISR 或高频路径中 → 替换为 ring buffer + 后台任务输出
> - 调试日志统一收口到一个 logger 模块，支持编译期裁剪等级




# 任务：按照 micro-ROS / ROS2 官方规范重构当前项目的命名

请扫描整个项目，按以下规范对所有命名进行重构。重构时**保持功能不变**，只修改命名、文件名、以及必要的引用更新。

## 一、命名规范（强制）

### C++ 代码
| 元素 | 规范 | 示例 |
|------|------|------|
| 类 / 结构体 | CamelCase | `LaserProcessor`、`PoseEstimator` |
| 函数 / 方法 | snake_case | `compute_velocity()`、`publish_odometry()` |
| 局部变量 | snake_case | `current_pose`、`max_speed` |
| 类成员变量 (private/protected) | snake_case + 后缀下划线 `_` | `velocity_`、`node_handle_`、`tf_buffer_` |
| 函数参数 | snake_case，**不加**下划线后缀 | `void set_speed(double max_speed)` |
| 常量 / constexpr | kCamelCase | `kMaxIterations`、`kDefaultTimeout` |
| 枚举值 | kCamelCase | `enum class State { kIdle, kRunning, kStopped }` |
| 宏 | UPPER_SNAKE_CASE | `RCLCPP_INFO`、`MY_PROJECT_DEBUG` |
| 命名空间 | snake_case | `namespace my_robot::sensors` |
| 模板参数 | CamelCase | `template <typename MessageT>` |
| 文件名 (.cpp/.hpp) | snake_case | `laser_processor.cpp`、`laser_processor.hpp` |
| 头文件保护宏 | `<PACKAGE>__<PATH>__<FILE>_HPP_` | `MY_PKG__SENSORS__LASER_PROCESSOR_HPP_` |

### Python 代码（PEP 8）
- 类：`CamelCase`
- 函数 / 变量 / 模块文件名：`snake_case`
- 常量：`UPPER_SNAKE_CASE`
- 私有成员：前缀单下划线 `_member`

### ROS2 资源命名
| 资源 | 规范 | 示例 |
|------|------|------|
| 包名 | snake_case，字母开头 | `my_robot_navigation` |
| 节点名 | snake_case | `laser_filter_node` |
| Topic / Service / Action 名 | snake_case，描述性 | `filtered_scan`、`reset_odometry` |
| 消息 / 服务 / 动作类型文件 | CamelCase.msg/.srv/.action | `LaserScan.msg`、`SetPose.srv` |
| 消息字段 | snake_case | `range_min`、`frame_id` |
| 参数名 | snake_case，分层用 `.` | `controller.max_velocity` |
| Frame ID（遵循 REP-105） | snake_case | `base_link`、`odom`、`map`、`laser_link` |

### CMake
- 命令小写：`find_package`、`add_executable`
- 变量 snake_case
- 2 空格缩进，禁用 tab

## 二、Topic 组织约定（采用 Autoware / micro-ROS 风格）

所有节点的 topic 必须放在私有命名空间下，并按用途分组：

```
node_name/
  ├── ~/input/<name>     # 订阅的输入
  ├── ~/output/<name>    # 发布的输出
  └── ~/debug/<name>     # 调试用
```

任何使用全局 topic（以 `/` 开头）的地方，必须在代码注释中说明理由。

## 三、命名内容要求

1. **描述性**：`state` → `planner_state`，`data` → `filtered_pointcloud`
2. **不要缩写**，除非是公认缩写（`tf`、`odom`、`imu`、`gps`、`url`、`id`）
3. **单复数**：消息类型用单数（`Pose` 而不是 `Poses`），topic 根据语义（多元素用复数：`detected_objects`）
4. **布尔变量 / 函数**用 `is_`、`has_`、`should_` 前缀：`is_initialized_`、`has_received_scan_`
5. **回调函数**统一用 `_callback` 后缀：`scan_callback()`、`timer_callback()`
6. **getter 不加 `get_` 前缀**（Google Style）：`velocity()` 而不是 `get_velocity()`；setter 保留 `set_`：`set_velocity()`

## 四、执行步骤

1. **先扫描**：列出当前项目中所有不符合规范的命名，按文件分组输出一份清单
2. **分类**：把改动分为
   - A. 仅内部改动（局部变量、私有成员）—— 低风险，直接改
   - B. 涉及对外接口（topic、参数、消息字段、公共 API）—— 高风险，需要确认
3. **等我确认 B 类改动后再统一执行**
4. 执行时使用安全的重命名方式（IDE 重构 / 全局替换 + 编译验证），每改完一个包跑一次 `colcon build` 确认不破坏构建
5. 修改完同步更新：launch 文件、yaml 参数文件、README、注释中的引用

## 五、自动化检查（重构完成后配置）

在项目根目录添加 / 更新以下配置，让规范自动强制执行：

1. `.clang-format`（基于 ROS2 官方版本，可从 `https://github.com/ament/ament_lint` 获取）
2. `.pre-commit-config.yaml`（参考 MoveIt2 / Nav2 的配置）
3. 在每个包的 `CMakeLists.txt` 中启用 `ament_lint_auto`：
```cmake
   if(BUILD_TESTING)
     find_package(ament_lint_auto REQUIRED)
     ament_lint_auto_find_test_dependencies()
   endif()
```
4. 在 `package.xml` 中添加 `<test_depend>ament_lint_common</test_depend>`

## 六、参考来源

- ROS2 官方代码风格：https://docs.ros.org/en/rolling/The-ROS2-Project/Contributing/Code-Style-Language-Versions.html
- REP-144（包名）：https://www.ros.org/reps/rep-0144.html
- REP-105（坐标系）：https://www.ros.org/reps/rep-0105.html
- Topic/Service 命名设计：https://design.ros2.org/articles/topic_and_service_names.html
- Nav2 源码作为代码组织参考：https://github.com/ros-navigation/navigation2

## 输出要求

- 第一步先输出扫描清单和重构计划，**不要直接动代码**
- 等我审核计划后再开始执行
- 每完成一个包，输出该包的改动摘要和构建测试结果