# SG90 上电状态机与握手待办事项

## 背景

SG90 舵机在 PWM 信号不稳定或控制端未对齐时容易抽搐，甚至打到机械极限。当前设计要求 STM32 上电后先输出中位 PWM，等待 500ms 后进入等待连接状态；只有树莓派 `uart_bridge_node` 发出初始化握手，且 STM32 回复 `ACTIVE` 后，双方才进入正常工作模式。

## 当前协议约定

- 协议版本：`UART_PROTOCOL_VERSION = 0x03`
- 树莓派到 STM32：`UART_CMD_INIT_HANDSHAKE = 0x10`
- STM32 到树莓派：`UART_CMD_SYSTEM_STATE = 0x83`
- 舵机控制：`UART_CMD_SERVO_CONTROL = 0x01`
- 舵机状态上报：`UART_CMD_SERVO_STATE_V2 = 0x82`
- STM32 状态：
  - `BOOT_CENTERING = 0x01`
  - `WAITING_CONNECTION = 0x02`
  - `ACTIVE = 0x03`
  - `ERROR = 0x7F`

## 必做待办

- [ ] 在 Keil/MDK 环境重新编译 STM32 工程，确认以下文件纳入工程并通过编译：
  - `stm32_keil/Core/Src/status_safety_task.c`
  - `stm32_keil/Core/Src/uart_rx_task.c`
  - `stm32_keil/Core/Src/servo_driver.c`
  - `stm32_keil/Core/Src/traj_planner.c`
  - `stm32_keil/Core/Inc/uart_protocol.h`

- [ ] 烧录 STM32 固件，断开树莓派 UART，仅给 STM32 和舵机上电，确认现象：
  - 上电后 4 路 PWM 立即输出中位。
  - 舵机保持约 90 度，不打到 0 度或 180 度。
  - 500ms 后 STM32 进入 `WAITING_CONNECTION`，但仍不执行舵机控制命令。

- [ ] 用示波器或逻辑分析仪确认 PA0-PA3 PWM：
  - 频率约 50Hz。
  - 中位脉宽约 1.5ms。
  - 上电后的前 500ms 内没有异常窄脉冲、宽脉冲或停止输出。

- [ ] 在树莓派部署更新后的 `uart_bridge` 包和协议头，重新编译：

```bash
cd ~/ros2_ws
colcon build --packages-select uart_bridge
source install/setup.bash
```

- [ ] 启动 `uart_bridge_node`，确认日志顺序合理：
  - 先打印已发送 STM32 初始化握手。
  - 如果 STM32 仍在 `BOOT_CENTERING`，bridge 应等待。
  - 收到 `ACTIVE` 后打印握手完成。
  - 握手完成前收到的 `/servo_cmd` 应被丢弃。

- [ ] 运行树莓派 UART 测试脚本：

```bash
python3 scripts/test_uart_rpi.py
```

验收标准：
- 脚本先完成初始化握手。
- 能看到 STM32 返回 `ACTIVE`。
- 后续能持续收到 `0x82` 舵机状态帧。
- 发送舵机命令后舵机动作正常。

## 负向测试

- [ ] 不启动树莓派，只让 STM32 和舵机上电 60 秒，确认舵机一直停在中位附近。
- [ ] 在未握手前手动发送 `0x01` 舵机控制帧，确认 STM32 不执行命令，并回复当前系统状态。
- [ ] 发送错误协议版本的 `0x10` 握手帧，确认 STM32 不进入 `ACTIVE`。
- [ ] 在 STM32 上电后 500ms 内发送握手帧，确认 STM32 先保持 `BOOT_CENTERING`，到 500ms 后才允许进入 `ACTIVE`。
- [ ] 重启 `uart_bridge_node`，不重启 STM32，确认 bridge 每次启动都会重新发送握手并重新对齐状态。

## 文档和测试同步

- [ ] 更新旧文档中仍写 `0x81` 为主要状态帧的说明，改为 `0x82`。
- [ ] 更新 `docs/task-2-1-uart.md` 的预期输出，加入握手步骤。
- [ ] 确认三份协议头保持一致：
  - `shared/uart_protocol.h`
  - `stm32_keil/Core/Inc/uart_protocol.h`
  - `ros2_ws/src/uart_bridge/include/uart_bridge/uart_protocol.h`
- [ ] 把本次握手流程补充进 `docs/uart-files.md`。

## 可选增强

- [ ] 在 `/servo_state` 或单独 diagnostics 话题发布 STM32 系统状态，便于 ROS 侧观察 `BOOT_CENTERING`、`WAITING_CONNECTION`、`ACTIVE`。
- [ ] 增加 STM32 连接超时策略：长时间没有收到有效命令时保持当前位置或回中位。
- [ ] 给 `uart_bridge_node` 的 UART 写操作加互斥锁，避免未来增加多处写帧后产生串口写交叠。
- [ ] 增加桌面端协议单元测试，覆盖错误握手版本、提前控制命令、系统状态帧解析。

## 已完成的本地验证

- `gcc -Wall -Wextra -Ishared tests/test_uart_protocol/test_crc.c shared/uart_protocol.c -o /tmp/test_crc && /tmp/test_crc`
- `gcc -Wall -Wextra -Ishared tests/test_uart_protocol/test_sizeof_servo_cmd_item.c -o /tmp/test_sizeof_servo_cmd_item && /tmp/test_sizeof_servo_cmd_item`
- `g++ -std=c++17 -Wall -Wextra -Iros2_ws/src/uart_bridge/include tests/test_uart_frame_codec.cpp ros2_ws/src/uart_bridge/src/frame_encoder.cpp ros2_ws/src/uart_bridge/src/frame_parser.cpp ros2_ws/src/uart_bridge/src/uart_protocol.c -o /tmp/test_uart_frame_codec && /tmp/test_uart_frame_codec`
- `python3 -m py_compile scripts/test_uart_rpi.py`
- `source /opt/ros/jazzy/setup.bash && colcon build --packages-select uart_bridge`

## 未完成原因

STM32 工程当前是 Keil/RVDS 工程，本机环境没有 Keil/armclang，不能在 WSL 内完成最终固件编译和烧录验收。硬件侧的 PWM 波形、舵机实际动作和树莓派 UART 联调需要在实物环境执行。
