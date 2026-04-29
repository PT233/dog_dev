# 时间同步

当前时间同步逻辑只存在于 Raspberry Pi 侧的 `uart_bridge`。

## 1. 输入来源

STM32 在 `SERVO_STATE_V2` 里给每个状态项附带：

- `timestamp_ms`
- `frame_seq`

对应定义见 `shared/uart_protocol.h` 中的 `ServoStateItem_v2`。

## 2. Pi 侧实现

文件：`ros2_ws/src/uart_bridge/src/uart_bridge_time_sync.cpp`

`TimestampMapper` 会缓存最近 20 条映射：

- `stm32_time_ms`
- `rpi_time`

## 3. 映射策略

### 只有一个映射点时

直接用 `delta_ms` 做线性偏移。

### 有多个映射点时

在相邻的两个样本之间做线性插值：

1. 找到包围当前 `stm32_time_ms` 的两个样本
2. 计算比例
3. 在对应的两个 `rpi_time` 之间插值

### 没有可用窗口时

退回到最新的接收时间。

## 4. 为什么这样做

- 不要求 STM32 与 Pi 的绝对时钟同步
- 只需要把 STM32 相对时间合理映射到 ROS 时间轴
- 成本低，适合当前 `50 ms` 周期状态回报

## 5. 限制

- `timestamp_ms` 只有 `uint16_t`，约 `65.5 s` 回绕一次
- 这是接收侧映射，不是全系统统一时钟同步
- 目前只用于 `/uart_bridge_node/output/servo_state` 时间戳和接收延迟统计
