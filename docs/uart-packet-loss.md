# UART 丢包检测

当前丢包检测只针对 `SERVO_STATE_V2` 回程链路。

## 1. 依据

STM32 在每个 `ServoStateItem_v2` 里都带有：

- `servo_id`
- `frame_seq`

Pi 侧 `FrameSequenceChecker` 会为每个 `servo_id` 记住上一次序号。

## 2. 检测逻辑

文件：`ros2_ws/src/uart_bridge/src/uart_bridge_time_sync.cpp`

对于每个舵机：

1. 第一次看到序号时只记住，不报错
2. 后续期望 `current_seq == last_seq + 1`
3. 如果序号跳变，就按差值累计 `drop_count`

## 3. 日志表现

在 `uart_bridge_protocol.cpp` 里，如果检测到 gap，会打印：

- `Frame loss detected on servo X: Y frames lost`

统计输出位于 `ReportStatistics()`：

- `Frames received`
- `Frame errors`
- `Total packet loss`
- `Latency UART RX`

## 4. 常见原因

- Pi 串口读写被阻塞
- STM32 发送过快但对端没有及时消费
- 线材或接地问题引起的串口误码
- 波特率设置不一致

## 5. 当前局限

- 这是回程状态帧的检测，不覆盖 Pi -> STM32 的控制命令方向
- 统计粒度是每个 `servo_id` 的序号链，不是整帧全局序号
