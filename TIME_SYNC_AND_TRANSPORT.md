# STM32 / 树莓派 / WSL2 三者时间同步与传输层分析

## 当前状态分析

### 现有时间戳处理
```
STM32 (无绝对时间)
    ↓ UART 921600 bps (1-2ms 延迟)
树莓派 (ROS2 系统时间 @ now())
    ↓ 接收时添加时间戳 (uart_bridge_node.cpp:149)
    ↓ network DDS (1-10ms 延迟)
WSL2 (Windows 系统时间)
```

**问题**：时间戳在树莓派接收端生成，不反映 STM32 发送时的真实时间。

---

## 一、时间同步方案（3个方案对比）

### 方案 A：无时间同步（当前）
```
优点：
  - 实现最简单（0 额外代码）
  - 足够用于相对时间序列（如速度计算）

缺点：
  - 无法进行跨系统时间戳关联
  - 调试困难（日志无法对齐）
  - 无法进行时间序列分析（谁先谁后不明确）

适用场景：
  - 实时控制环路（本地反馈）
  - 目标跟踪（相对时间足够）
```

### 方案 B：树莓派作为时间基准（推荐）
```
架构：
  1. STM32 不维护绝对时间，只通过 UART 发送相对时间戳（microseconds 或 FreeRTOS ticks）
  2. 树莓派接收到 UART 帧时，记录两个时间戳：
     - t_receive_rpi = now()  (ROS2 系统时间)
     - t_stm32_local = frame.timestamp (STM32 本地相对时间)
  3. 树莓派建立映射：t_stm32_local → t_rpi_absolute
  4. 树莓派发布 /servo_state 时，使用映射后的时间戳

实现步骤：
  
  步骤 1：扩展 UART 协议
  ───────────────────
  struct ServoStateItem {
      uint8_t servo_id;
      int16_t current_angle_x10;
      uint8_t status;
      uint32_t timestamp_us;  // ← 新增：STM32 本地微秒时间戳
  }
  
  注意：
    - 32-bit 微秒 @ 72MHz 主频，溢出周期 ~59 秒
    - 可以接受（每 60 秒重置一次偏移映射）
    - 或使用 uint16_t milliseconds（更简单，溢出周期 ~65 秒）
  
  步骤 2：STM32 固件修改
  ──────────────────────
  // uart_rx_task.c
  uint32_t get_stm32_timestamp_us(void) {
      // 使用 FreeRTOS tick，转换为微秒
      return xTaskGetTickCount() * (1000000 / configTICK_RATE_HZ);
  }
  
  // 在发送 UART_CMD_SERVO_STATE 时，添加时间戳
  ServoStateItem items[4];
  for (int i = 0; i < 4; i++) {
      items[i].timestamp_us = get_stm32_timestamp_us();
  }
  
  步骤 3：树莓派端时间映射（uart_bridge_node.cpp）
  ─────────────────────────────────────────────────
  class TimestampMapper {
  private:
    struct Mapping {
        uint32_t stm32_time;  // STM32 本地时间
        rclcpp::Time rpi_time;  // 树莓派时间
    };
    std::deque<Mapping> mappings_;  // 保留最近 N 个映射点
    std::mutex mutex_;
    static const size_t MAX_MAPPINGS = 10;
  
  public:
    void RecordMapping(uint32_t stm32_time) {
        auto now = rclcpp::Clock(RCL_SYSTEM_TIME).now();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            mappings_.push_back({stm32_time, now});
            if (mappings_.size() > MAX_MAPPINGS) {
                mappings_.pop_front();
            }
        }
    }
    
    rclcpp::Time MapTimestamp(uint32_t stm32_time) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (mappings_.empty()) return rclcpp::Clock(RCL_SYSTEM_TIME).now();
        
        // 线性插值：假设 STM32 和树莓派时钟频率相同
        // 找到最近的两个映射点
        if (mappings_.size() == 1) {
            // 单点映射：直接计算偏差
            uint32_t time_diff = stm32_time - mappings_[0].stm32_time;
            return mappings_[0].rpi_time + rclcpp::Duration(0, time_diff * 1000);
        }
        
        // 多点映射：线性插值
        for (size_t i = 1; i < mappings_.size(); i++) {
            if (stm32_time >= mappings_[i-1].stm32_time && 
                stm32_time <= mappings_[i].stm32_time) {
                // 在 i-1 和 i 之间插值
                uint32_t dt = mappings_[i].stm32_time - mappings_[i-1].stm32_time;
                if (dt == 0) return mappings_[i].rpi_time;
                
                double ratio = (double)(stm32_time - mappings_[i-1].stm32_time) / dt;
                rclcpp::Duration elapsed = mappings_[i].rpi_time - mappings_[i-1].rpi_time;
                return mappings_[i-1].rpi_time + 
                       rclcpp::Duration(0, (int64_t)(elapsed.nanoseconds() * ratio));
            }
        }
        
        // 超出范围：使用最近的映射点
        return mappings_.back().rpi_time;
    }
  };
  
  步骤 4：修改 OnFrameReceived
  ────────────────────────────
  void OnFrameReceived(uint8_t cmd_id, const uint8_t* payload, size_t len) {
    if (cmd_id == UART_CMD_SERVO_STATE) {
        auto state_msg = std::make_shared<sensor_msgs::msg::JointState>();
        
        // 记录接收时间点（用于后续映射）
        const ServoStateItem* first_item = reinterpret_cast<const ServoStateItem*>(payload);
        timestamp_mapper_.RecordMapping(first_item->timestamp_us);
        
        // 使用映射后的时间戳
        state_msg->header.stamp = timestamp_mapper_.MapTimestamp(first_item->timestamp_us);
        state_msg->header.frame_id = "";
        
        // ... 其余代码不变
    }
  }

成本：
  - UART 协议：+4 字节 per ServoStateItem (但可以压缩到 2 字节)
  - 树莓派代码：~50 行
  - STM32 代码：~5 行

优点：
  - 精度：±1-5ms（取决于 STM32 时钟精度）
  - 可调试（日志时间对齐）
  - 支持端到端延迟分析

缺点：
  - 需要修改协议
  - 时钟漂移（STM32 无外部晶振同步）
```

### 方案 C：精确时间同步（PTP / NTP）
```
架构：树莓派定期向 STM32 发送时间校准包

实现复杂度：★★★★☆ (中高)
精度：μs 级

步骤：
  1. 添加新命令：UART_CMD_TIME_SYNC (0x03)
  2. 树莓派每秒发送一次：[Header, 0x03, Len, Timestamp_64bit, CRC, Tail]
  3. STM32 接收后，计算往返时间，调整 systick 偏差

成本：
  - 带宽：~20 bytes/sec
  - 实现：~200 行代码（双端）

适用场景：
  - 需要跨系统的全局时间戳
  - 复杂的多节点协调
  - 事件日志关联分析
```

**推荐**：采用 **方案 B** - 树莓派作为时间基准，兼顾精度、成本、实现复杂度。

---

## 二、传输层面的关键问题

### 1. 乱序问题（Frame Out-of-Order）

**问题描述**：
```
STM32 -> 树莓派：正常有序（单链路，不存在乱序）
树莓派 -> WSL2 (DDS)：可能乱序（多个UDP包）

场景：
  - /servo_cmd 从视觉伺服节点发送
  - 可能通过多条网络路径到达树莓派
  - 树莓派接收不到的也很少（局域网）
  - 但确实存在可能性
```

**防护**：
```cpp
// uart_bridge_node 订阅 /servo_cmd 时，应记录序列号
struct ServoCmdMsg {
    uint32_t seq_num;  // 序列号
    JointState cmd;
};

// 或使用 ROS2 内置的 sequence ID（Header.stamp 已经有了）
// /servo_cmd 的 header.stamp 可作为序列标识
```

### 2. 丢包问题（Frame Loss）

**问题分析**：
```
UART 丢包场景（STM32 -> 树莓派）：
  1. DMA 缓冲区溢出（uart_rx_task.c 中的 256 字节缓冲）
  2. 树莓派串口读取速度不足
  3. 硬件故障

ROS2 丢包场景（树莓派 -> WSL2）：
  1. 网络拥塞
  2. QoS 设置不当
  3. WSL2 接收端处理不及时
```

**当前保护**：
```c
// uart_rx_task.c 中有计数器
volatile uint32_t uart_crc_error_count  = 0;  // CRC 错误
volatile uint32_t uart_queue_drop_count = 0;  // 队列丢包

// uart_bridge_node.cpp 使用 SetErrorCallback 记录解析错误
parser_->SetErrorCallback([this](const char* msg) {
    RCLCPP_WARN(this->get_logger(), "Parse error: %s", msg);
});
```

**改进建议**：
```cpp
// 1. 添加帧序列号到协议中
typedef struct __attribute__((packed)) {
    uint8_t servo_id;
    int16_t current_angle_x10;
    uint8_t status;
    uint16_t frame_seq;  // ← 新增：序列号，检测丢包
} ServoStateItem;

// 2. 树莓派端检测丢包
class FrameSequenceChecker {
private:
    uint16_t last_seq_ = 0;
    uint32_t drop_count_ = 0;
    
public:
    bool CheckSequence(uint16_t seq) {
        if (seq != (last_seq_ + 1) % 65536) {
            drop_count_ += (seq - last_seq_ - 1 + 65536) % 65536;
            RCLCPP_WARN(this->get_logger(), 
                       "Frame loss detected: expected %u, got %u (total drops: %u)",
                       last_seq_ + 1, seq, drop_count_);
            return false;
        }
        last_seq_ = seq;
        return true;
    }
};
```

### 3. 延迟分析（End-to-End Latency）

**当前延迟构成**：
```
STM32 发送 /servo_state
    ↓ 1-2 ms (UART 传输 @ 921600 bps, 50 字节)
树莓派接收并解析
    ↓ 0.5-1 ms (uart_bridge_node 处理)
树莓派发布 /servo_state 到 DDS
    ↓ 1-10 ms (网络传输 + DDS 中间件)
WSL2 接收 /servo_state
    ↓ 1-2 ms (visual_servo_node 处理)
WSL2 发送 /servo_cmd
    ↓ 1-10 ms (网络传输 + DDS)
树莓派接收 /servo_cmd
    ↓ 0.5-1 ms (uart_bridge_node 处理)
树莓派通过 UART 发送到 STM32
    ↓ 1-2 ms (UART 传输)
STM32 执行舵机控制

总延迟：6-29 ms，典型值 ~12-15 ms
```

**延迟测量**：
```cpp
// 方案：在协议中添加时间戳，跟踪端到端延迟
class LatencyMonitor {
private:
    struct LatencySample {
        rclcpp::Time t_stm32_send;
        rclcpp::Time t_rpi_receive;
        rclcpp::Time t_rpi_send;
        rclcpp::Time t_wsl2_receive;
        rclcpp::Time t_wsl2_send;
        rclcpp::Time t_rpi_receive_cmd;
    };
    std::deque<LatencySample> samples_;
    std::mutex mutex_;
    
public:
    void RecordSample(const LatencySample& sample) {
        std::lock_guard<std::mutex> lock(mutex_);
        samples_.push_back(sample);
        if (samples_.size() > 100) samples_.pop_front();
        
        rclcpp::Duration uart_latency = sample.t_rpi_receive - sample.t_stm32_send;
        rclcpp::Duration dds_latency = sample.t_wsl2_receive - sample.t_rpi_send;
        rclcpp::Duration total_latency = sample.t_rpi_receive_cmd - sample.t_stm32_send;
        
        RCLCPP_DEBUG(this->get_logger(),
            "Latencies - UART: %.2f ms, DDS: %.2f ms, Total: %.2f ms",
            uart_latency.nanoseconds() / 1e6,
            dds_latency.nanoseconds() / 1e6,
            total_latency.nanoseconds() / 1e6);
    }
};
```

### 4. CRC 和数据完整性

**当前实现**：
```
✓ CRC16-CCITT (0x1021 多项式)
✓ 每帧都进行 CRC 校验
✓ CRC 错误统计 (uart_crc_error_count)
✓ 坏帧直接丢弃

潜在问题：
  1. 单个 CRC16 检测率 ~99.998%（概率 1/65536）
  2. 如果通信不稳定，可能积累多个错误
  3. 没有重传机制
```

**改进建议**：
```
1. 添加帧计数器（已在建议 #2 中涉及）
2. 考虑添加简单的 ARQ 重传
3. 定期发送健康检查帧

typedef struct __attribute__((packed)) {
    uint8_t servo_id;
    int16_t current_angle_x10;
    uint8_t status;
    uint16_t frame_seq;      // 序列号
    uint16_t frame_checksum; // CRC16
    uint8_t reserved;        // 保留
} ServoStateItem_v2;  // 升级版本
```

### 5. 流量控制与背压（Backpressure）

**问题**：
```
如果 WSL2 处理速度 < 树莓派发送速度会发生什么？

当前流程：
  uart_bridge_node 以 50Hz 发送 /servo_state
  如果订阅端处理不及时，ROS2 DDS 会堆积消息

防护机制：
  - uart_bridge_node 使用 SensorDataQoS（depth=5）
  - visual_servo_node 使用 reliable() QoS
```

**监测方法**：
```bash
# 监测队列深度
rclcpp::QueueSizeCallback = [this](size_t depth) {
    if (depth > 3) {
        RCLCPP_WARN(this->get_logger(), "DDS queue depth: %zu", depth);
    }
};

# 命令行监测
ros2 node info /uart_bridge_node
ros2 topic hz /servo_state
```

---

## 三、完整改进方案（阶段性）

### 阶段 1：立即可做（不修改协议）
```
1. ✓ 添加时间戳映射器到 uart_bridge_node
   - 文件：uart_bridge_node.cpp
   - 改动：~50 行
   - 效果：可精确关联 STM32 和树莓派的时间
   
2. ✓ 添加延迟监测
   - 文件：uart_bridge_node.cpp + behavior_node/visual_servo_node
   - 改动：~100 行
   - 效果：实时延迟数据，便于调试
   
3. ✓ 增强错误统计
   - 文件：uart_bridge_node.cpp
   - 改动：~20 行
   - 效果：统计丢包率、CRC 错误率
```

### 阶段 2：短期改进（修改协议 v2）
```
1. 添加帧序列号（2 字节）
   - 检测丢包
   
2. 添加 STM32 时间戳（2 字节毫秒）
   - 精确的时间同步映射
   
3. 添加状态字节扩展
   - 电压、温度、错误码
   
新协议大小：+4-6 字节 per item
向后兼容：通过 CMD_ID 版本号区分
```

### 阶段 3：长期（系统级）
```
1. 实现 PTP 时间同步
2. 添加完整的健康检查帧
3. 实现 UART 重传机制
4. 加入 TLS 加密（如需要）
```

---

## 四、推荐行动方案

### 当前系统是否够用？
```
✓ 对于视觉伺服（目标跟踪）：充分
✓ 对于 30Hz 控制频率：充分
✓ 对于精确延迟分析：不足
✓ 对于故障诊断：不足
```

### 立即推荐
```
1. 实现方案 B 的时间戳映射器
   - 工作量：1 小时
   - 收益：完整的时间戳关联，便于调试
   
2. 添加帧计数和丢包检测
   - 工作量：1 小时
   - 收益：掌握系统可靠性
   
3. 添加端到端延迟监测
   - 工作量：2 小时
   - 收益：性能优化基础数据
```

### 缓后推荐
```
协议 v2（阶段 2）
  - 不紧急，但有良好的改进空间
  - 等第一阶段稳定后再做
```

---

## 五、代码实现清单

### uart_bridge_node.cpp 改进
```cpp
// 增加文件头
#include <deque>
#include <mutex>

// 新增类：TimestampMapper（方案 B）
class TimestampMapper { ... };  // ~60 行

// 修改 OnFrameReceived
void OnFrameReceived(uint8_t cmd_id, const uint8_t* payload, size_t len) {
    timestamp_mapper_.RecordMapping(first_item->timestamp_us);
    state_msg->header.stamp = timestamp_mapper_.MapTimestamp(first_item->timestamp_us);
    // ...
}

// 新增：LatencyMonitor
class LatencyMonitor { ... };  // ~50 行

// 新增：FrameSequenceChecker
class FrameSequenceChecker { ... };  // ~30 行
```

### shared/uart_protocol.h 未来扩展
```c
// 版本 1（当前）
typedef struct __attribute__((packed)) {
    uint8_t servo_id;
    int16_t current_angle_x10;
    uint8_t status;
} ServoStateItem;  // 4 字节

// 版本 2（计划）
typedef struct __attribute__((packed)) {
    uint8_t servo_id;
    int16_t current_angle_x10;
    uint8_t status;
    uint16_t frame_seq;      // +2 字节
    uint16_t timestamp_ms;   // +2 字节
    // 总计：8 字节
} ServoStateItem_v2;
```

---

## 总结

| 问题 | 当前状态 | 风险等级 | 建议行动 |
|-----|--------|--------|--------|
| 时间同步 | 无 | ⚠️ 中 | 实现方案 B（时间戳映射） |
| 乱序检测 | 无 | ⚠️ 低 | 添加序列号检测（阶段 2） |
| 丢包检测 | CRC 统计 | ⚠️ 中 | 添加帧序列号检测 |
| 延迟分析 | 无 | ⚠️ 中 | 实现延迟监测 |
| 数据完整性 | CRC16 | ✅ 高 | 保持当前方案 |
| 流量控制 | QoS | ✅ 高 | 保持当前方案 |

**系统可靠性等级**：`B+` → 可升级到 `A`（实现阶段 1）
