# UART 相关文件地图

## 1. 共享协议层

| 文件 | 作用 |
| --- | --- |
| `shared/uart_protocol.h` | UART 协议主定义 |
| `shared/uart_protocol.c` | CRC16 实现 |

## 2. Raspberry Pi / ROS 2 侧

| 文件 | 作用 |
| --- | --- |
| `ros2_ws/src/uart_bridge/src/uart_bridge_main.cpp` | 单节点入口 |
| `ros2_ws/src/uart_bridge/src/uart_bridge_node_internal.hpp` | 节点内部声明 |
| `ros2_ws/src/uart_bridge/src/uart_bridge_transport.cpp` | 串口打开、读写、topic 桥接 |
| `ros2_ws/src/uart_bridge/src/uart_bridge_protocol.cpp` | 握手、帧解码、状态发布 |
| `ros2_ws/src/uart_bridge/src/uart_bridge_time_sync.cpp` | 时间戳映射、丢包统计 |
| `ros2_ws/src/uart_bridge/src/frame_parser.cpp` | 字节流解析状态机 |
| `ros2_ws/src/uart_bridge/src/frame_encoder.cpp` | 帧编码 |
| `ros2_ws/src/uart_bridge/config/uart_bridge.yaml` | 串口参数 |

## 3. STM32 侧

| 文件 | 作用 |
| --- | --- |
| `stm32_keil/Core/Inc/uart_protocol.h` | STM32 协议镜像 |
| `stm32_keil/Core/Src/uart_rx_task.c` | DMA + IDLE 收包与命令分发 |
| `stm32_keil/Core/Src/status_safety_task.c` | `SERVO_STATE_V2` 与 `SYSTEM_STATE` 回传 |
| `stm32_keil/Core/Src/traj_planner.c` | 轨迹规划 |
| `stm32_keil/Core/Src/servo_driver.c` | PWM 输出 |

## 4. 相关工具

| 文件 | 作用 |
| --- | --- |
| `scripts/test_uart_rpi.py` | Pi 侧串口测试 |
| `scripts/mock_uart_bridge.py` | 无硬件时模拟 `/uart_bridge_node/output/servo_state` |
| `tests/test_uart_frame_codec.cpp` | 编解码测试 |
| `tests/test_uart_frame_parser.cpp` | 状态机解析测试 |
