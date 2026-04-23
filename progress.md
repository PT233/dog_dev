# 任务进度记录

最后更新：2026-04-23（任务 3.1 已完成）
当前执行策略：按 task.md 顺序逐项完成；每完成一个任务后自动执行验收测试。

## 当前状态
- 总体状态：进行中
- 当前阶段：阶段 3（图像推流链路）
- 当前任务：3.2（树莓派 H.264 硬件编码 + UDP 推流）
- 状态：任务 3.1 已完成 ✅

## 已完成任务
| 任务 | 状态 | 结果摘要 | 验收结果 |
| --- | --- | --- | --- |
| 0.1 | 已完成 | 创建基础目录结构与 .gitkeep，新增 README 一句话描述 | 结构检查通过 |
| 0.2 | 已完成 | 完成 shared/uart_protocol.h，共享协议常量/枚举/结构体/CRC 原型齐备 | 自动测试 PASS（头文件编译 + sizeof(ServoCmdItem)==5） |
| 0.3 | 已完成 | 实现 shared/uart_protocol.c 的 CRC16-CCITT，并新增 test_crc.c 五个测试用例 | 自动测试 PASS（5/5） |
| 1.1 | 已完成 | 最小修复并完成 STM32 工程基础配置：启用 IWDG、开启 USART1 IDLE 中断、修复 GCC10 链接脚本兼容 | 自动测试 PASS（mingw32-make 构建成功并生成 elf/hex/bin） |
| 1.2 | 已完成 | 新增 servo_driver.c/h，提供 4 路舵机角度 API；main.c 加入 servo0 每秒 0°/180° 摆动测试；Makefile 纳入新源文件 | 自动测试 PASS（mingw32-make 构建成功并生成 elf/hex/bin） |
| 1.3 | 已完成 | 新增 UART 协议接收任务：DMA 循环接收 + IDLE 中断通知 + 帧切分 + CRC 校验 + `uart_rx_queue` 投递 | 自动测试 PASS（mingw32-make 构建成功并生成 elf/hex/bin） |
| 1.4 | 已完成 | 新增 traj_planner.c/h：TrajState 结构体 + 对称梯形速度曲线 + 5ms 周期 FreeRTOS 任务；freertos.c 注册任务并在 defaultTask 中做 0°↔180° 往复测试 | 自动测试 PASS（make clean && make 构建成功并生成 elf/hex/bin） |
| 1.5 | 已完成 | uart_rx_task 从 queue 消费 ServoCmdItem 并调用 Traj_SetTarget + 更新 uart_last_cmd_tick；新增 status_safety_task.c/h，含 Task_Status_TX（50ms 发 0x81 帧）和 Task_Safety（100ms 喂 IWDG） | 自动测试 PASS（make clean && make 构建成功并生成 elf/hex/bin，text 21036 B） |
| 2.1 | 已完成 | 创建 scripts/test_uart_rpi.py：支持 921600bps UART 通信，能发送舵机控制帧并接收状态反馈 | 树莓派端验证通过 |
| 2.2 | 已完成 | 创建 robot_interfaces 包：TargetInfo.msg + SetTargetClass.srv + CalibrateCenter.srv + CMakeLists.txt + package.xml | 自动测试 PASS（colcon build 成功，所有接口生成正常） |
| 2.3 | 已完成 | 创建 uart_bridge ROS 2 C++ 包骨架：uart_bridge_node.cpp 继承 rclcpp::Node，构造函数打印启动信息 | 自动测试 PASS（colcon build 成功，ros2 run 正常启动并打印日志） |
| 2.4 | 已完成 | uart_bridge 打开串口并持续读：加入 POSIX termios 串口读写，参数化设备路径和波特率，单独线程读取并打印字节数 | 自动测试 PASS（colcon build 成功，launch 正确加载参数，节点优雅处理设备缺失） |
| 2.5 | 已完成 | uart_bridge 实现帧解析：状态机解析器（WAIT_HEADER→GOT_AA→GOT_55→READ_LEN→READ_PAYLOAD→READ_CRC→READ_TAIL），CRC 校验，收到 0x81 帧时打印 servo_state 信息 | 自动测试 PASS（colcon build 成功无警告，单元测试 4/4 通过：解析有效帧、拒绝坏帧、恢复垃圾字节） |
| 2.6 | 已完成 | uart_bridge 订阅 /servo_cmd 并下发：创建 FrameEncoder 类编码 ServoCmdItem；订阅 /servo_cmd (JointState) 用 SensorDataQoS；JointState.name→servo_id 映射；位置(弧度)→角度×10 转换；duration_ms=100 硬编码 | 自动测试 PASS（colcon build 成功，编码单元测试 4/4 通过，集成测试 8/8 通过，包括角度转换和 servo ID 映射） |
| 2.7 | 已完成 | uart_bridge 发布 /servo_state：创建 Publisher `/servo_state` (JointState) QoS=reliable,depth=10；收到 0x81 帧时解析 ServoStateItem[]；构造 JointState：name=["yaw","pitch","s2","s3"]，position[]为弧度，header.stamp=now() | 自动测试 PASS（colcon build 成功，发布集成测试 7/7 通过：QoS 配置、消息构造、名称映射、角度转换、发布调用） |
| 3.1 | 已完成 | 树莓派相机取流验证（纯 GStreamer）：验证 /dev/video0 存在，YUYV 格式 640×480@30fps；camera.sh 脚本执行成功启用双目模式；GStreamer 管道成功运行，帧率 30fps > 20fps 需求 | 树莓派端验证通过（摄像头就绪，双目模式启用，GStreamer 管道运行正常） |

## 阶段进度
- 阶段 0：3/3 ✅
- 阶段 1：5/5 ✅
- 阶段 2：7/7 ✅
- 阶段 3：1/7
- 阶段 4：0/6
- 阶段 5：0/3
- 阶段 6：0/5
- 阶段 7：0/6

## 维护规则
- 每次仅推进一个任务。
- 若任务验收失败，先最小修复后再重测。
- 任务通过后再进入下一个任务。

## 阻塞记录
- 2026-04-23：任务 1.1 已生成工程，但自动校验未通过。
	- 可用构建命令：`mingw32-make -j4`（`make` 命令不存在）。
	- 链接失败：`STM32F103XX_FLASH.ld` 使用 `READONLY` 语法，当前 GCC10 链接器不兼容。
	- 配置缺失：未启用 IWDG，且未发现 USART1 IDLE 中断使能代码。
- 2026-04-23：任务 1.1 阻塞已解除。
	- 已完成最小修复后通过自动构建验收。
