# 上位机视觉识别到机器人动作的数据流追踪

## 先说结论

按当前仓库代码，"识别到目标物体，进而发一个移动指令，机器人怎么动起来" 这条链，最终驱动的是 **4 路机器人腿部 SG90 舵机**，不是轮式底盘，也不是云台。

- 已实现链路：`视觉检测/跟踪 -> 像素误差 -> 前进/转向控制 -> /servo_cmd -> UART -> STM32 -> PWM -> 4 路腿部 SG90`
- 未发现链路：`/cmd_vel -> 电机驱动 -> 轮子`

代码里没有 `cmd_vel`、`geometry_msgs/Twist`、轮电机驱动或编码器闭环控制实现。这里的"动起来"，当前准确说法是：**固定摄像头提供视觉输入，4 条腿舵机根据目标位置生成步态动作**。

---

## 1. 总链路

前端视觉栈由 [`ros2_ws/src/robot_bringup/launch/vision_stack.launch.py`](../ros2_ws/src/robot_bringup/launch/vision_stack.launch.py) 组织，启动顺序是：

1. `gst_receiver`
2. `stereo_splitter`
3. `detection_node`
4. `tracker_node`
5. `behavior_node`
6. `visual_servo`

对应代码见 `generate_launch_description()`，文件：`ros2_ws/src/robot_bringup/launch/vision_stack.launch.py:7-48`。

树莓派侧只起 UART 桥，见 [`ros2_ws/src/robot_bringup/launch/rpi_stack.launch.py`](../ros2_ws/src/robot_bringup/launch/rpi_stack.launch.py)。

完整数据流：

```text
Raspberry Pi 摄像头/H.264 UDP
  -> gst_receiver_node
  -> /stereo/image_raw (sensor_msgs/Image, bgr8)
  -> stereo_splitter_node
  -> /camera/image_mono (sensor_msgs/Image, 320x480, bgr8)
  -> detection_node
  -> /detections (SimpleDetection2DArray)
  -> tracker_node
  -> /tracked_objects (SimpleDetection2DArray, 带 track_id)
  -> behavior_node
  -> /pixel_error (geometry_msgs/Vector3)
  -> leg_motion_node
  -> /servo_cmd (sensor_msgs/JointState, rad)
  -> uart_bridge_node
  -> UART 二进制帧 (ServoCmdItem[])
  -> STM32 uart_rx_task
  -> traj_planner
  -> servo_driver
  -> TIM2 PWM 50Hz
  -> SG90 舵机动作
```

反馈闭环：

```text
STM32 status_safety_task
  -> UART ServoStateItem_v2[]
  -> uart_bridge_node
  -> /servo_state (sensor_msgs/JointState, rad)
  -> leg_motion_node
```

---

## 2. 分步骤追踪

| 步骤 | 节点/模块 | 关键文件和函数 | 输入 | 输出 | 实时性关注 |
| --- | --- | --- | --- | --- | --- |
| 1 | 视频接收 | `gst_receiver_node.cpp` / `GstReceiverNode::on_new_sample()` | H.264 RTP/UDP | `/stereo/image_raw` | `rtpjitterbuffer latency=50`，`appsink drop=true` |
| 2 | 左目裁剪 | `stereo_splitter_node.cpp` / `StereoSplitterNode::image_callback()` | `/stereo/image_raw` | `/camera/image_mono` | 逐行 memcpy，轻量 |
| 3 | 目标检测 | `detection_node.cpp` / `ImageCallback()`、`InferenceWorker()` | `/camera/image_mono` | `/detections` | 推理队列深度 2，丢旧帧保新鲜度 |
| 4 | 目标跟踪 | `tracker_node.cpp` / `OnDetections()` | `/detections` | `/tracked_objects` | 取决于输入帧率 |
| 5 | 行为决策 | `behavior_node.cpp` / `OnTrackedObjects()`、`SelectTarget()` | `/tracked_objects` | `/pixel_error` | 必须尽快，把像素误差交给控制环 |
| 6 | 腿部控制 | `visual_servo_node.cpp` / `OnControlTimer()` | `/pixel_error` + `/servo_state` | `/servo_cmd` | 30 Hz 控制环 |
| 7 | 上位机串口编码 | `uart_bridge_node.cpp` / `OnServoCmdReceived()`、`WriteFrame()` | `/servo_cmd` | UART 字节流 | 未握手前直接丢命令 |
| 8 | 下位机收包 | `uart_rx_task.c` / `Task_UART_RX()`、`UartRx_HandleFrame()` | UART 字节流 | `ServoCmdItem` 队列 | DMA + IDLE 中断，队列长 8 |
| 9 | 轨迹规划 | `traj_planner.c` / `Traj_SetTarget()`、`Traj_Update()` | `ServoCmdItem` | 5ms 中间角度 | 5 ms 更新周期 |
| 10 | PWM 输出 | `servo_driver.c` / `Servo_SetAngle()` | 目标角度（度） | TIM2 CCR | 50 Hz PWM |
| 11 | 状态反馈 | `status_safety_task.c` / `StatusTX_SendFrame()` | `g_traj_state[]` | UART 状态帧 | 20 Hz 上报 |

---

## 3. 每一步到底做了什么

### 3.1 视频进入 ROS

文件：`ros2_ws/src/gst_receiver/src/gst_receiver_node.cpp`

- `GstReceiverNode::try_build_pipeline()` 组装 GStreamer pipeline，固定输出 `BGR`，见 `53-100`。
- `GstReceiverNode::on_new_sample()` 把 `GstBuffer` 转成 `sensor_msgs::msg::Image`，发布到 `/stereo/image_raw`，见 `125-164`。

数据格式转换：

```text
H.264 RTP/UDP
  -> GStreamer 解码
  -> cv::Mat(height, width, CV_8UC3, ...)
  -> sensor_msgs/Image
     encoding = "bgr8"
```

关键时序：

- pipeline 里显式设置了 `rtpjitterbuffer latency=50`，这一步天然引入约 50 ms 缓冲。
- `appsink max-buffers=2 drop=true`，说明设计目标是低延迟优先，不追求每帧必达。

### 3.2 双目图像裁左半边

文件：`ros2_ws/src/stereo_splitter/src/stereo_splitter_node.cpp`

- `StereoSplitterNode::image_callback()` 订阅 `/stereo/image_raw`，取左半边，输出 `/camera/image_mono`，见 `18-45`。

数据格式转换：

```text
sensor_msgs/Image (stereo, bgr8)
  -> 手工截取左半边 [0,320) x [0,480)
  -> sensor_msgs/Image (320x480, bgr8)
```

实现细节：

- 这里没有做灰度化，虽然 topic 名叫 `/camera/image_mono`，实际编码仍然是 `bgr8`。

关键时序：

- 只是逐行 `memcpy`，CPU 开销小，不是主要瓶颈。

### 3.3 YOLO 检测

文件：`ros2_ws/src/detection_node/src/detection_node.cpp`

- `ImageCallback()` 把输入图像压入推理队列，见 `61-73`。
- `InferenceWorker()` 单独线程做推理并发布 `/detections`，见 `75-131`。

数据格式转换：

```text
sensor_msgs/Image
  -> cv::Mat
  -> YoloInfer::Infer(img)
  -> robot_interfaces::msg::SimpleDetection2DArray
```

`SimpleDetection.msg` 字段：

```text
float32 center_x
float32 center_y
float32 width
float32 height
float32 confidence
int32   class_id
string  track_id
```

这里 `track_id` 在检测阶段被填成空字符串。

关键时序：

- 队列深度硬编码为 2；推理慢时新帧不会无限堆积。
- 这一步通常是整条链最大的时延来源。

### 3.4 ByteTrack 跟踪

文件：

- `ros2_ws/src/tracker_node/src/tracker_node.cpp`
- `ros2_ws/src/tracker_node/src/byte_tracker.cpp`

关键函数：

- `TrackerNode::OnDetections()`：`31-76`
- `TrackerNode::SimpleDetectionToByteTrack()`：`78-88`
- `ByteTracker::Update()`：`6-87`

数据格式转换：

```text
SimpleDetection2DArray
  -> std::vector<tracker_node::Detection>
  -> ByteTracker::Update()
  -> SimpleDetection2DArray (重新挂上 track_id)
```

逻辑：

- 先把 ROS 消息转成内部 `Detection {x,y,w,h,conf,class_id}`。
- 用简化版 ByteTrack 做贪心 IoU 匹配。
- 输出时把 `track_id` 写回 `SimpleDetection.track_id` 字段。

关键时序：

- 算法本身较轻。
- 它的价值不在低层实时，而在于给行为层一个连续目标 ID。

### 3.5 行为层把"识别结果"变成"控制误差"

文件：`ros2_ws/src/behavior_node/src/behavior_node.cpp`

关键函数：

- `OnTrackedObjects()`：`48-74`
- `SelectTarget()`：`76-107`

逻辑：

1. 订阅 `/tracked_objects`
2. 只保留当前目标类别 `target_class_id_` 的候选
3. 在同类目标里选面积最大的框
4. 计算目标中心相对画面中心的偏差
5. 发布 `/pixel_error`

数据格式转换：

```text
SimpleDetection2DArray
  -> 选中一个 detection
  -> geometry_msgs/Vector3
     x = target_cx - center_x
     y = target_cy - center_y
     z = 0
```

这里 `/pixel_error` 不是速度命令，也不是角度命令，而是 **像素坐标误差**。

关键时序：

- 这一层本身几乎不耗时。
- 但它位于控制环前面，误差发布时间越拖，30 Hz 控制环用到的数据就越旧。

### 3.6 腿部控制：像素误差变成四腿舵机目标角

文件：

- `ros2_ws/src/visual_servo/src/visual_servo_node.cpp`
- `ros2_ws/src/visual_servo/src/pid_controller.cpp`

关键函数：

- `OnPixelError()`：`77-80`
- `OnServoState()`：`82-91`
- `OnControlTimer()`：`93-135`
- `PIDController::Update()`：`13-46`

控制逻辑：

1. 订阅 `/pixel_error`
2. 订阅 `/servo_state`，拿当前 4 条腿反馈角
3. 每 30 Hz 定时运行一次控制环
4. `pixel_error_x` 生成转向控制量 `turn_bias`
5. `pixel_error_y` 生成前进控制量 `stride_command`
6. 控制节点内部推进一个步态相位 `gait_phase`
7. 将 `turn_bias` 和 `stride_command` 合成为 4 条腿的目标角
8. 发布 `/servo_cmd`

数据格式转换：

```text
geometry_msgs/Vector3 (pixel_error)
  + sensor_msgs/JointState (/servo_state, rad)
  -> stride / turn command (deg)
  -> sensor_msgs/JointState (/servo_cmd, rad)
```

`/servo_cmd` 内容：

- `name = {"front_left", "front_right", "rear_left", "rear_right"}`
- `position = {fl_rad, fr_rad, rl_rad, rr_rad}`

注意：

- 固定摄像头本身不参与控制。
- `visual_servo` 这个包名仍然保留，但节点内部逻辑已经改成四腿控制器。

关键时序：

- 这是真正的上位机控制环，周期约 `33 ms`。
- `PIDController::Update()` 里有死区、积分饱和、微分项，目的是抑制舵机抖动和超调。

### 3.7 UART 桥：ROS JointState 变成串口协议

文件：

- `ros2_ws/src/uart_bridge/include/uart_bridge/uart_protocol.h`
- `ros2_ws/src/uart_bridge/src/frame_encoder.cpp`
- `ros2_ws/src/uart_bridge/src/frame_parser.cpp`
- `ros2_ws/src/uart_bridge/src/uart_bridge_node.cpp`

关键函数：

- `UartBridgeNode::OnServoCmdReceived()`：`499-554`
- `FrameEncoder::EncodeServoControl()`：`6-16`
- `FrameEncoder::BuildFrame()`：`40-73`
- `UartBridgeNode::WriteFrame()`：`564-584`

ROS 到串口 payload 的转换：

```text
sensor_msgs/JointState
  name: ["front_left","front_right","rear_left","rear_right"]
  position: [rad, rad, rad, rad]

  -> 名称映射为 servo_id
     front_left=0, front_right=1, rear_left=2, rear_right=3

  -> 弧度转角度
     angle_deg = rad * 180 / pi

  -> 角度乘 10
     angle_x10 = int16_t(angle_deg * 10)

  -> 封成 ServoCmdItem
     {servo_id, angle_x10, duration_ms=100}
```

协议定义见 `uart_protocol.h:24-79`。

帧格式：

```text
[0xAA][0x55][CMD_ID][LEN][PAYLOAD...][CRC16_LO][CRC16_HI][0x0D]
```

这里运动控制命令的 `CMD_ID = 0x01 (UART_CMD_SERVO_CONTROL)`。

如果一帧发 4 路舵机：

- `ServoCmdItem` 大小 = 5 字节
- payload = `4 * 5 = 20` 字节
- 总帧长 = `2 + 1 + 1 + 20 + 2 + 1 = 27` 字节

关键时序：

- `uart_bridge` 在 STM32 握手完成前会直接丢弃 `/servo_cmd`，见 `500-505`。
- 握手包每 `500 ms` 重发一次，直到 STM32 进入 `ACTIVE`，见 `274-283`、`350-365`。
- 921600 bps 下，27 字节控制帧的发送时间大约是亚毫秒级，串口本身不是主要瓶颈。

### 3.8 STM32 收包、验帧、入队

文件：`stm32_keil/Core/Src/uart_rx_task.c`

关键函数：

- `UartRx_HandleFrame()`：`32-97`
- `UartRx_ParseByte()`：`99-160`
- `Task_UART_RX()`：`180-207`
- `UartRxTask_NotifyFromIdleIrq()`：`228-244`

逻辑：

1. USART1 以 DMA 循环接收
2. 收到 IDLE 中断后通知 `Task_UART_RX`
3. 任务把 DMA 环形缓冲区里的字节逐个喂给帧解析器
4. 校验帧头、长度、CRC、帧尾
5. 如果是握手包，交给 `StatusSafety_HandleInitHandshake()`
6. 如果是控制包，且系统已 `ACTIVE`，则逐个 `ServoCmdItem` 入队
7. 再由 `Task_UART_RX` 调 `Traj_SetTarget()`

数据格式转换：

```text
UART byte stream
  -> 完整 frame
  -> ServoCmdItem
  -> FreeRTOS Queue
  -> Traj_SetTarget(id, angle_deg, duration_ms)
```

关键时序：

- 这里是下位机最敏感的一段接收路径。
- 用的是 `DMA + IDLE`，比轮询稳得多。
- 队列长度只有 `8`；如果上位机短时间塞很多包，可能触发 `uart_queue_drop_count++`。

### 3.9 轨迹规划：串口命令变成平滑运动

文件：`stm32_keil/Core/Src/traj_planner.c`

关键函数：

- `TrajPlanner_Init()`：`30-43`
- `Traj_SetTarget()`：`45-74`
- `Traj_Update()`：`77-125`
- `Task_Traj_Planner()`：`127-136`

逻辑：

- `Traj_SetTarget()` 收到目标角和总时长后，不是立刻跳到目标角。
- 它会构造一条梯形速度曲线：
  - 前 30% 时间加速
  - 中间 40% 时间匀速
  - 后 30% 时间减速
- `Task_Traj_Planner()` 每 `5 ms` 更新一次中间角度。

数据格式转换：

```text
ServoCmdItem
  {servo_id, angle_x10, duration_ms}
  -> target_deg = angle_x10 / 10
  -> TrajState
  -> 每 5 ms 的 current_angle
```

关键时序：

- `TRAJ_TICK_MS = 5`，这是下位机运动平滑度的核心节拍。
- `duration_ms` 现在由上位机固定写成 `100 ms`，所以每次新命令都会让舵机按 100 ms 规划移动。

### 3.10 PWM 输出：中间角度变成定时器比较值

文件：

- `stm32_keil/Core/Src/servo_driver.c`
- `stm32_keil/Core/Src/tim.c`

关键函数：

- `Servo_Init()`：`46-53`
- `Servo_SetAngle()`：`55-70`
- `MX_TIM2_Init()`：`30-84`

数据格式转换：

```text
angle_deg
  -> clamp 到 [0,180]
  -> ccr = 500 + angle_deg * (2000 / 180)
  -> __HAL_TIM_SET_COMPARE(TIM2_CHx, ccr)
  -> PWM 脉宽 0.5ms ~ 2.5ms
  -> SG90 转到对应角度
```

硬件参数：

- TIM2 预分频 `71`，计数周期 `19999`，对应 `1 MHz` 计数时钟、`20 ms` 周期
- 也就是标准舵机 PWM：`50 Hz`

关键时序：

- 这一层已经很接近硬件。
- 5 ms 轨迹更新和 20 ms PWM 周期叠加后，舵机看到的是连续更新的目标脉宽。

### 3.11 反馈闭环：STM32 再把状态送回上位机

文件：

- `stm32_keil/Core/Src/status_safety_task.c`
- `ros2_ws/src/uart_bridge/src/uart_bridge_node.cpp`

关键函数：

- STM32: `StatusTX_SendFrame()`：`84-118`
- STM32: `Task_Status_TX()`：`145-159`
- 树莓派: `OnFrameReceived()`：`404-496`

下位机到上位机的数据格式转换：

```text
g_traj_state[i].current_angle
  -> ServoStateItem_v2
     {servo_id, current_angle_x10, status, timestamp_ms, frame_seq}
  -> UART frame (CMD_ID=0x82)
  -> uart_bridge 解析
  -> sensor_msgs/JointState (/servo_state, rad)
```

`/servo_state` 被 `leg_motion_node` 用来更新 4 条腿当前角度，从而构成闭环。

关键时序：

- 状态上报周期 `50 ms`，即 `20 Hz`。
- 这意味着上位机 30 Hz 控制环使用的角度反馈，刷新率低于控制率。
- `uart_bridge` 还维护了时间戳映射、延迟统计、丢帧统计，见 `uart_bridge_node.cpp:18-176`。

---

## 4. 关键数据格式转换总表

### 4.1 图像链

```text
H.264 RTP/UDP
  -> sensor_msgs/Image("/stereo/image_raw", bgr8)
  -> sensor_msgs/Image("/camera/image_mono", 320x480, bgr8)
  -> cv::Mat
```

### 4.2 视觉理解链

```text
cv::Mat
  -> SimpleDetection2DArray
     {
       header,
       detections: [
         {
           center_x, center_y,
           width, height,
           confidence,
           class_id,
           track_id=""
         }
       ]
     }
  -> SimpleDetection2DArray(带 track_id)
  -> geometry_msgs/Vector3 pixel_error
```

### 4.3 控制链

```text
geometry_msgs/Vector3
  -> 前进/转向控制量 (deg)
  -> sensor_msgs/JointState
     {
       name: ["front_left","front_right","rear_left","rear_right"],
       position: [rad, rad, rad, rad]
     }
```

### 4.4 通信链

```text
sensor_msgs/JointState
  -> ServoCmdItem[]
     {
       servo_id: uint8_t,
       angle_x10: int16_t,
       duration_ms: uint16_t
     }
  -> UART frame bytes
```

### 4.5 下位机执行链

```text
ServoCmdItem
  -> TrajState
  -> current_angle (deg)
  -> TIM2 CCR
  -> PWM pulse width
  -> SG90 机械转动
```

---

## 5. 关键时序约束

按实时性从高到低看：

### A. 下位机执行环是最硬的一层

- `Task_UART_RX()` 必须及时把 DMA 缓冲区里的字节消费掉，否则会积压。
- `Task_Traj_Planner()` 固定 `5 ms` 更新一次，这是实际运动平滑性的底座。
- `Servo_SetAngle()` 直接改 TIM2 CCR，必须尽量短小。

### B. 上位机控制环次之

- `leg_motion_node` 以 `30 Hz` 定时运行。
- 如果 `/pixel_error` 更新太慢，控制环会拿旧误差做 PID。
- 如果 `/servo_state` 回传太慢，控制环就更像半闭环。

### C. 感知链强调低延迟而不是不丢帧

- GStreamer `appsink drop=true`
- 检测队列深度 2

这两个设计都说明：系统宁愿丢旧帧，也不愿让控制命令基于过时目标位置。

### D. 握手是控制使能门槛

- 上位机每 `500 ms` 发一次 `INIT_HANDSHAKE`
- STM32 启动后先 `BOOT_CENTERING 500 ms`
- 进入 `WAITING_CONNECTION` 后才可能切到 `ACTIVE`
- `ACTIVE` 之前，树莓派侧直接丢 `/servo_cmd`

### E. 状态反馈比控制环慢

- 状态上报 `20 Hz`
- 控制环 `30 Hz`

这会带来一个直接结果：不是每个控制周期都能拿到新的机械反馈。

---

## 6. 真正让机器人"动起来"的最后三跳

如果只看最关键的末端链路，就是这三步：

1. `leg_motion_node::OnControlTimer()`  
   把像素误差算成四腿舵机目标角，发布 `/servo_cmd`

2. `uart_bridge_node::OnServoCmdReceived()` + `Task_UART_RX()`  
   把 `JointState` 变成 `ServoCmdItem`，经 UART 送到 STM32，再交给 `Traj_SetTarget()`

3. `Traj_Update()` + `Servo_SetAngle()`  
   每 5 ms 生成中间角度，再写 TIM2 CCR，输出 50 Hz PWM，舵机物理转动

---

## 7. 这条链里最容易误解的点

### 7.1 "移动指令" 不是底盘速度指令

当前代码里的"移动指令"实际是：

- `sensor_msgs/JointState /servo_cmd`
- 目标是 `front_left/front_right/rear_left/rear_right`
- 本质是 **舵机角度命令**

不是：

- `geometry_msgs/Twist`
- 左右轮转速
- 电机占空比

### 7.2 `/camera/image_mono` 名字和内容不一致

- topic 名叫 `image_mono`
- 实际编码是 `bgr8`

对当前检测节点没影响，因为它按彩色图处理。

### 7.3 当前控制的是四条腿

`leg_motion_node` 真正闭环控制的是：

- `front_left`
- `front_right`
- `rear_left`
- `rear_right`

---

## 8. 一句话版本

上位机识别到目标后，并不是直接给电机一个"前进/左转"命令；它先把目标位置变成画面中心误差，再经 30 Hz 控制环变成四条腿舵机目标角，通过 UART 二进制协议发给 STM32，STM32 再用 5 ms 轨迹规划把角度命令转成 TIM2 的 50 Hz PWM，最终让 4 路 SG90 腿舵机动作起来。
