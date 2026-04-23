# 构建任务清单（task.md）

> **使用方式**：把每个任务作为一个独立 prompt 交给工程 LLM。完成并通过"验收标准"后，再执行下一个任务。**不要跳步**，不要一次交多个任务。

> **任务命名规则**：`阶段号.任务号`，例如 `1.3` 表示阶段 1 的第 3 个任务。

> **图例**：
> - 🖥️ = 在 WSL2 执行
> - 🍓 = 在树莓派执行
> - 🔧 = 在 STM32 CubeIDE 执行
> - 📝 = 纯文档/配置任务，无需硬件

---

## 目录

- [阶段 0：基础设施与协议定义](#阶段-0基础设施与协议定义)
- [阶段 1：STM32 固件](#阶段-1stm32-固件)
- [阶段 2：树莓派 UART 桥](#阶段-2树莓派-uart-桥)
- [阶段 3：图像推流链路](#阶段-3图像推流链路)
- [阶段 4：YOLOv8 检测](#阶段-4yolov8-检测)
- [阶段 5：跟踪器](#阶段-5跟踪器)
- [阶段 6：决策与视觉伺服](#阶段-6决策与视觉伺服)
- [阶段 7：集成与调参](#阶段-7集成与调参)

---

## 阶段 0：基础设施与协议定义

### 📝 任务 0.1：创建仓库基本结构

**目标**：建立空仓库和顶层目录。

**输入**：无

**执行内容**：
1. 创建 git 仓库 `desktop_tracking_robot`
2. 创建如下空目录：
   ```
   ros2_ws/src/
   stm32_fw/
   shared/
   models/
   config/
   scripts/
   tests/
   docs/
   ```
3. 在每个目录放一个 `.gitkeep` 文件
4. 创建顶层 `README.md`，写一句话项目描述

**验收标准**：
- `git status` 干净
- `tree -L 2` 显示上述目录结构
- `README.md` 能在 GitHub 正常渲染

---

### 📝 任务 0.2：编写共享 UART 协议头文件

**目标**：定义树莓派和 STM32 共用的二进制协议。

**输入**：无

**执行内容**：
创建 `shared/uart_protocol.h`，内容必须包含：
- 帧头 `0xAA 0x55`、帧尾 `0x0D` 宏定义
- `CMD_ID` 枚举：`0x01` 舵机控制、`0x02` 查询、`0x81` 状态反馈、`0xFF` 急停
- `ServoCmdItem` 结构体（servo_id + angle_x10 + duration_ms），`__attribute__((packed))`
- `ServoStateItem` 结构体（servo_id + current_angle_x10 + status）
- CRC16-CCITT 计算函数原型
- 最大帧长常量 `UART_MAX_FRAME_LEN = 256`

**验收标准**：
- 头文件能被 C 和 C++ 代码同时 include（用 `#ifdef __cplusplus extern "C"`）
- 用 `gcc -Wall -Wextra -c` 编译空的测试文件无警告
- `sizeof(ServoCmdItem) == 5`（用一个独立的 test 文件验证）

---

### 📝 任务 0.3：实现 CRC16 函数并写单元测试

**目标**：实现协议里的 CRC16-CCITT 算法。

**输入**：任务 0.2 的头文件

**执行内容**：
1. 创建 `shared/uart_protocol.c`，实现 `crc16_ccitt(const uint8_t* data, size_t len)`
2. 创建 `tests/test_uart_protocol/test_crc.c`，测试用例至少 5 个：
   - 空输入
   - 单字节 `0x00`
   - 已知答案："123456789" 的 CRC16-CCITT = `0x29B1`
   - 随机 16 字节
   - 协议完整帧

**验收标准**：
- `gcc tests/test_uart_protocol/test_crc.c shared/uart_protocol.c -o test_crc && ./test_crc`
- 所有测试用例输出 `PASS`

---

## 阶段 1：STM32 固件

### 🔧 任务 1.1：创建 STM32 CubeMX 工程

**目标**：用 CubeMX 生成 STM32F103C8T6 的基础工程，包含 FreeRTOS、UART、PWM。

**输入**：无

**执行内容**：
1. CubeMX 新建工程，选 STM32F103C8T6
2. 时钟配置：HSE 8MHz 外部晶振，PLL 倍频到 72MHz
3. 启用 FreeRTOS（CMSIS_V2 接口）
4. 配置外设：
   - USART1：921600 bps，8N1，开启 DMA（TX + RX）、开启 IDLE 中断
   - TIM2 CH1~CH4：4 路 PWM 输出，频率 50Hz（prescaler=71, period=19999）
   - IWDG：超时 1 秒
5. 生成 Makefile 工程到 `stm32_fw/`
6. 不写任何业务代码，直接编译通过

**验收标准**：
- `cd stm32_fw && make` 无错误
- 烧录后板子上 LED（如有）能闪烁（FreeRTOS 默认 Idle Task 测试）

---

### 🔧 任务 1.2：编写 servo_driver.c/h

**目标**：提供"设置某路舵机到某角度"的底层 API。

**输入**：任务 1.1 工程

**执行内容**：
1. 创建 `stm32_fw/Core/Inc/servo_driver.h` 和 `Src/servo_driver.c`
2. API：
   ```c
   void Servo_Init(void);
   void Servo_SetAngle(uint8_t servo_id, float angle_deg);  // 直接设 PWM
   float Servo_GetAngle(uint8_t servo_id);                   // 返回上次设的值
   ```
3. 角度转 CCR 公式：`ccr = 500 + angle_deg * (2000/180)`（0.5ms~2.5ms 对应 0~180°）
4. 在 `main.c` 的测试段落：循环让 servo 0 在 0°~180° 之间每秒切换

**验收标准**：
- 烧录后 SG90 舵机（servo 0 接 PA0）每秒摆动一次
- 用示波器量 PA0 脉冲宽度 0.5ms ↔ 2.5ms 切换

---

### 🔧 任务 1.3：编写 UART 协议接收任务

**目标**：用 DMA + IDLE 中断接收变长帧，校验 CRC，解析后放入队列。

**输入**：任务 0.2/0.3 头文件，任务 1.1 工程

**执行内容**：
1. 在 `stm32_fw/Core/` 加入 `shared/uart_protocol.c/h`（用 symlink 或复制）
2. 新建 `uart_rx_task.c`：
   - 启动 DMA 循环接收到 256 字节 buffer
   - IDLE 中断触发时，读取 DMA 计数器得到已收字节数
   - 按 `0xAA 0x55 ... 0x0D` 切分帧
   - CRC 校验，失败则丢弃并计数
   - 合法帧放入 FreeRTOS queue `uart_rx_queue`
3. 创建任务 `Task_UART_RX`，优先级中等
4. 在 `main.c` 启动任务

**验收标准**：
- 用串口助手（USB-TTL 连 USART1）发一条合法舵机控制帧
- 调试器查看 `uart_rx_queue` 能看到解析后的 `ServoCmdItem`
- 发一条 CRC 错误帧，计数器递增但 queue 不入队

---

### 🔧 任务 1.4：编写梯形速度插值任务

**目标**：实现 `Traj_Planner_Task`，将"目标角度 + 时长"转成每 5ms 的中间角度。

**输入**：任务 1.2 servo_driver

**执行内容**：
1. 新建 `traj_planner.c/h`
2. 为每路舵机维护结构体：
   ```c
   typedef struct {
       float current_angle;
       float target_angle;
       float velocity;           // deg/s
       float max_accel;          // deg/s²
       uint32_t start_tick;
       uint32_t duration_ms;
   } TrajState;
   ```
3. API：`Traj_SetTarget(uint8_t id, float target, uint16_t duration_ms)`
4. `Task_Traj_Planner` 每 5ms 调用一次更新：
   - 梯形曲线：加速段 → 匀速段 → 减速段
   - 每次调用 `Servo_SetAngle(id, current_angle)`
5. 在 `main.c` 测试：调用 `Traj_SetTarget(0, 180, 2000)` 让 servo 0 在 2 秒内从 0° 转到 180°

**验收标准**：
- 舵机明显平滑移动，无抖动
- 用示波器看 PWM 占空比线性变化（实际是近似梯形）
- 更改 duration 能改变移动速度

---

### 🔧 任务 1.5：连通 UART 接收到轨迹规划

**目标**：把任务 1.3 收到的 `ServoCmdItem` 喂给任务 1.4 的 `Traj_SetTarget`。

**输入**：任务 1.3、1.4

**执行内容**：
1. `Task_UART_RX` 从 queue 取到 `ServoCmdItem` 后，直接调用 `Traj_SetTarget(item.servo_id, item.angle_x10/10.0f, item.duration_ms)`
2. 新增 `Task_Status_TX`：每 50ms 把 4 路当前角度打包成 CMD_ID=0x81 的帧，通过 UART 发出
3. 新增 `Task_Safety`：若 500ms 未收到合法舵机命令，保持当前位置（不做事，只是计数器清零由 UART_RX 负责）；同时定期喂 IWDG

**验收标准**：
- 串口助手发送"舵机 0 转到 90° 耗时 1000ms"的帧 → 舵机平滑移动
- 同时串口助手能持续收到状态反馈帧（20Hz）
- 断开串口 500ms 后再连，舵机保持原位不乱动

---

## 阶段 2：树莓派 UART 桥

### 🍓 任务 2.1：树莓派 UART 物理连通性测试

**目标**：确认 `/dev/ttyAMA0` 可用，和 STM32 正常通信（不涉及 ROS）。

**输入**：STM32 已烧录任务 1.5 固件

**执行内容**：
1. 修改 `/boot/firmware/config.txt` 加入 `dtoverlay=disable-bt` 和 `enable_uart=1`
2. `sudo systemctl disable hciuart`
3. reboot 后 `ls -l /dev/ttyAMA0` 应存在
4. 用 `minicom` 或 Python pyserial 脚本：
   - 以 921600 bps 打开 `/dev/ttyAMA0`
   - 发送一个完整的舵机控制帧（硬编码字节序列）
   - 读取返回的状态反馈帧

**验收标准**：
- Python 脚本执行后 STM32 上的舵机动起来
- 脚本能持续读到 STM32 发来的 0x81 状态帧

---

### 🍓 任务 2.2：创建 ros2_ws 工作空间和 robot_interfaces 包

**目标**：建立 ROS 2 工作空间，创建自定义消息包。

**输入**：树莓派已装 ROS 2 Jazzy Base

**执行内容**：
1. `cd ros2_ws && mkdir -p src`
2. `cd src && ros2 pkg create --build-type ament_cmake robot_interfaces`
3. 在 `robot_interfaces` 下：
   - `msg/TargetInfo.msg`（字段见 architecture.md § 6.2）
   - `srv/SetTargetClass.srv`
   - `srv/CalibrateCenter.srv`
4. 修改 `CMakeLists.txt` 和 `package.xml` 支持 msg/srv 生成
5. `colcon build --packages-select robot_interfaces`

**验收标准**：
- `ros2 interface show robot_interfaces/msg/TargetInfo` 正常输出
- `ros2 interface list | grep robot_interfaces` 能看到 3 个接口

---

### 🍓 任务 2.3：创建 uart_bridge 包（空骨架）

**目标**：创建 ROS 2 C++ 节点骨架，能启动但不做事。

**输入**：任务 2.2 工作空间

**执行内容**：
1. `ros2 pkg create --build-type ament_cmake --dependencies rclcpp sensor_msgs uart_bridge`
2. 写 `src/uart_bridge_node.cpp`：
   - 继承 `rclcpp::Node`，节点名 `uart_bridge_node`
   - 构造函数中打印 "uart_bridge_node started"
   - `main()` 使用 `rclcpp::spin`
3. 编辑 CMakeLists.txt 生成可执行文件
4. `colcon build`

**验收标准**：
- `ros2 run uart_bridge uart_bridge_node` 正常启动
- 日志打印节点启动信息
- Ctrl+C 正常退出

---

### 🍓 任务 2.4：uart_bridge 打开串口并持续读

**目标**：节点启动时打开 `/dev/ttyAMA0`，循环读取并打印收到的字节数（先不解析）。

**输入**：任务 2.3 骨架

**执行内容**：
1. 加入 Boost.Asio 或 termios 串口读写代码
2. 在单独的线程里 `read()` 循环，每次读到数据打印 `"Got N bytes"`
3. 参数化：串口设备路径、波特率（从 YAML 读取）
4. 创建 `config/uart_bridge.yaml`

**验收标准**：
- 启动节点后，让 STM32 上电（它会每 50ms 发状态帧）
- 节点持续打印 "Got N bytes"，N 和协议帧长度一致
- 卸载 STM32 节点不崩溃，只是不再打印

---

### 🍓 任务 2.5：uart_bridge 实现帧解析

**目标**：把任务 2.4 的字节流解析成合法的协议帧，丢弃坏帧。

**输入**：任务 2.4 + `shared/uart_protocol.h`

**执行内容**：
1. 加入 `shared/uart_protocol.c/h` 到 uart_bridge 包
2. 实现状态机解析器：WAIT_HEADER → GOT_AA → GOT_55 → READ_LEN → READ_PAYLOAD → READ_CRC → READ_TAIL
3. 合法帧调用回调（当前只打印 "Got servo_state: id=X angle=Y"）
4. 坏帧（CRC 错或 tail 错）打印警告并继续

**验收标准**：
- STM32 持续发状态帧 → 节点持续打印正确的 id 和 angle
- 人为断开再接线（制造垃圾字节）→ 节点能从错误中恢复，不卡死

---

### 🍓 任务 2.6：uart_bridge 订阅 /servo_cmd 并下发

**目标**：订阅 ROS 话题 `/servo_cmd`（JointState），编码成协议帧，从串口发出。

**输入**：任务 2.5

**执行内容**：
1. 订阅 `/servo_cmd` (sensor_msgs/JointState)，QoS 为 `SensorDataQoS`
2. 回调中：
   - 对每个 name[i] 查表得到 servo_id（"yaw"→0, "pitch"→1, "s2"→2, "s3"→3）
   - 把 position[i]（弧度）转角度 × 10
   - 构造 `ServoCmdItem[]` payload + CMD_ID=0x01
   - 加 header / LEN / CRC / tail 写入串口
3. 硬编码 duration_ms = 100

**验收标准**：
- 终端 1 运行 uart_bridge
- 终端 2 执行：
  ```bash
  ros2 topic pub --once /servo_cmd sensor_msgs/JointState \
    "{name: ['yaw'], position: [1.5708]}"
  ```
- STM32 上 servo 0 转到约 90°

---

### 🍓 任务 2.7：uart_bridge 发布 /servo_state

**目标**：把任务 2.5 解析出的状态帧发成 ROS 话题。

**输入**：任务 2.5 + 2.6

**执行内容**：
1. 创建 Publisher：`/servo_state` (sensor_msgs/JointState)，QoS=reliable, depth=10
2. 收到 CMD_ID=0x81 帧时：
   - 构造 JointState：name=["yaw","pitch","s2","s3"], position=[...]（弧度）
   - 填充 header.stamp = `this->now()`
   - 发布

**验收标准**：
- 运行节点后，`ros2 topic echo /servo_state` 持续显示 4 路角度
- 频率 ~20Hz（用 `ros2 topic hz /servo_state` 验证）
- 手动让 `/servo_cmd` 转动舵机，`/servo_state` 的 position 对应变化

---

## 阶段 3：图像推流链路
可以通过ssh连接 ssh ubuntu@192.168.137.100 密码是123,相机是3D webcam，这个相机需要用脚本才能开启双目：
在树莓派的
### 🍓 任务 3.1：树莓派相机取流验证（纯 GStreamer）

**目标**：不涉及 ROS，用 `gst-launch-1.0` 验证 USB 双目摄像头能取到图像。

**输入**：USB 双目摄像头接在树莓派上

**执行内容**：
1. `ls /dev/video*` 找到摄像头设备
2. `v4l2-ctl --device=/dev/video0 --list-formats-ext` 确认支持的格式
3. 运行 GStreamer 预览管道（需要桌面环境或导出到 X11）：
   ```bash
   gst-launch-1.0 v4l2src device=/dev/video0 ! \
     image/jpeg,width=640,height=480,framerate=30/1 ! jpegdec ! \
     videoconvert ! autovideosink
   ```

**验收标准**：
- 能在树莓派本地或 X forwarding 看到双目拼接图像
- 帧率不低于 20 fps（看 `fpsdisplaysink` 数据）

---

### 🍓 任务 3.2：树莓派 H.264 硬件编码 + UDP 推流

**目标**：编写脚本 `scripts/start_camera_stream.sh`，用 `v4l2h264enc` 硬编码推流。

**输入**：任务 3.1 确认摄像头可用

**执行内容**：
1. 创建 `scripts/start_camera_stream.sh`：
   ```bash
   #!/bin/bash
   TARGET_IP=${1:-192.168.10.1}
   gst-launch-1.0 -v v4l2src device=/dev/video0 ! \
     image/jpeg,width=640,height=480,framerate=30/1 ! jpegdec ! \
     v4l2h264enc ! 'video/x-h264,level=(string)4' ! h264parse ! \
     rtph264pay config-interval=1 ! udpsink host=$TARGET_IP port=5600
   ```
2. `chmod +x` 并测试运行

**验收标准**：
- 脚本运行无错误
- 树莓派 CPU 占用低于 15%（用 `htop` 查看）
- `tcpdump -i eth0 udp port 5600` 能看到持续的 UDP 数据包

---

### 🖥️ 任务 3.3：WSL2 侧 GStreamer 拉流显示

**目标**：用 WSL2 的 GStreamer 拉取任务 3.2 的流，显示图像。

**输入**：任务 3.2 正在运行

**执行内容**：
1. 确认 WSL2 能显示图形（WSLg 或 X server）
2. 运行：
   ```bash
   gst-launch-1.0 udpsrc port=5600 caps="application/x-rtp, media=video, \
     encoding-name=H264, payload=96" ! rtpjitterbuffer ! \
     rtph264depay ! avdec_h264 ! videoconvert ! autovideosink
   ```

**验收标准**：
- WSL2 弹出窗口显示实时双目拼接图像
- 手动晃动摄像头，画面无明显延迟（估 <200ms）
- 短暂断网再恢复，画面能自动恢复

---

### 🖥️ 任务 3.4：创建 gst_receiver_node 包（空骨架）

**目标**：建 ROS 2 C++ 包，依赖 gstreamer-1.0。

**输入**：WSL2 的 ros2_ws

**执行内容**：
1. `ros2 pkg create --build-type ament_cmake --dependencies rclcpp sensor_msgs cv_bridge gst_receiver`
2. `package.xml` 加入 `gstreamer` 依赖
3. CMakeLists.txt 中 `find_package(PkgConfig) / pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)`
4. 空节点：构造函数打印 "gst_receiver_node started"

**验收标准**：
- `colcon build --packages-select gst_receiver` 无错误
- `ros2 run gst_receiver gst_receiver_node` 启动成功

---

### 🖥️ 任务 3.5：gst_receiver_node 内部嵌入 GStreamer 管道

**目标**：节点内部启动 GStreamer 管道，通过 `appsink` 拉帧。

**输入**：任务 3.4

**执行内容**：
1. 节点构造时：
   - `gst_init()`
   - 用 `gst_parse_launch()` 创建管道字符串（和任务 3.3 一致，但末尾改为 `appsink name=sink emit-signals=true`）
   - 注册 `new-sample` signal 回调
2. 回调中：
   - `gst_app_sink_pull_sample()` 取帧
   - `gst_buffer_map()` 取字节
   - 打印 "Got frame: WxH, size=N" 然后释放
3. 不做 ROS 发布，只验证能取帧

**验收标准**：
- 节点启动后持续打印 "Got frame: 640x480, size=921600"
- 帧率约 30 Hz

---

### 🖥️ 任务 3.6：gst_receiver_node 发布 ROS 图像话题

**目标**：把 GStreamer 帧转成 `sensor_msgs/Image` 发布。

**输入**：任务 3.5

**执行内容**：
1. 加 Publisher：`/stereo/image_raw` (sensor_msgs/Image)，QoS=SensorDataQoS
2. appsink 回调中：
   - 用 cv_bridge 或手动填充 `sensor_msgs::msg::Image`
   - encoding="bgr8", width=640, height=480
   - header.stamp 用 RTP PTS 换算（先用 `this->now()` 简化，后续优化）
3. 发布

**验收标准**：
- `ros2 topic hz /stereo/image_raw` 显示 ~30Hz
- `rqt_image_view` 选 `/stereo/image_raw` 能看到实时画面

---

### 🖥️ 任务 3.7：stereo_splitter_node 切分双目图像

**目标**：订阅拼接图，切出左半发成新话题。

**输入**：任务 3.6

**执行内容**：
1. `ros2 pkg create stereo_splitter`（依赖 rclcpp, sensor_msgs, cv_bridge, opencv）
2. 订阅 `/stereo/image_raw`
3. 回调中：
   - cv_bridge 转 cv::Mat
   - 切 `cv::Rect(0, 0, 320, 480)` 得到左半（注意：双目模式下拼接图左半对应左相机）
   - cv_bridge 转回 sensor_msgs/Image
   - 发布到 `/camera/image_mono`

**验收标准**：
- `rqt_image_view` 看 `/camera/image_mono` 是 320×480 的左相机图像
- 原始和切分后图像时间戳一致

---

## 阶段 4：YOLOv8 检测

### 🖥️ 任务 4.1：WSL2 Python 验证 YOLOv8 + CUDA

**目标**：确认 ultralytics 能在 WSL2 用 GPU 推理，不涉及 ROS。

**输入**：WSL2 + NVIDIA 驱动 + CUDA

**执行内容**：
1. 安装 `pip install ultralytics`
2. 写 `tests/test_detection/test_yolo_gpu.py`：
   ```python
   from ultralytics import YOLO
   import torch
   assert torch.cuda.is_available(), "CUDA not available"
   model = YOLO("yolov8n.pt")
   results = model.predict("https://ultralytics.com/images/bus.jpg", device=0)
   print(f"Inference device: {results[0].boxes.data.device}")
   ```
3. 运行脚本

**验收标准**：
- 脚本输出 "Inference device: cuda:0"
- 推理时间 <30ms（第二次推理后）
- `nvidia-smi` 能看到 Python 进程

---

### 🖥️ 任务 4.2：导出 YOLOv8n 为 ONNX

**目标**：生成 `models/yolov8n.onnx` 文件，供 C++ 使用。

**输入**：任务 4.1 环境

**执行内容**：
1. 写 `scripts/export_yolo_onnx.py`：
   ```python
   from ultralytics import YOLO
   model = YOLO("yolov8n.pt")
   model.export(format="onnx", opset=12, simplify=True, dynamic=False, imgsz=640)
   ```
2. 运行，把生成的 `yolov8n.onnx` 移到 `models/`
3. 生成 `models/coco_classes.txt`（80 行类别名，每行一个）

**验收标准**：
- `models/yolov8n.onnx` 存在，大小 ~6MB
- 用 Netron 打开能看到输入 [1,3,640,640]，输出 [1,84,8400]
- `models/coco_classes.txt` 80 行

---

### 🖥️ 任务 4.3：C++ ONNX Runtime 离线推理 demo

**目标**：写一个独立 C++ 程序，加载 ONNX 模型，推理一张静态图片。

**输入**：任务 4.2 的 onnx 文件

**执行内容**：
1. 安装 ONNX Runtime GPU 版（libonnxruntime-dev 或从 github 下载）
2. 写 `tests/test_detection/test_onnx_cpp.cpp`：
   - 加载 yolov8n.onnx，启用 CUDAExecutionProvider
   - 加载 `bus.jpg`，letterbox 到 640×640
   - 前向推理
   - 打印第一个输出张量的形状和前 10 个值
3. 用 CMake 编译

**验收标准**：
- 编译通过
- 运行输出张量形状 [1, 84, 8400]
- 推理时间 <30ms（预热后）
- `nvidia-smi` 能看到进程

---

### 🖥️ 任务 4.4：封装 YoloInfer 类

**目标**：把任务 4.3 的代码封装成可复用的类，放在 detection_node 包中。

**输入**：任务 4.3 + 先创建 detection_node 包骨架

**执行内容**：
1. `ros2 pkg create detection_node`（依赖 rclcpp, sensor_msgs, cv_bridge, vision_msgs, OpenCV）
2. 写 `include/detection_node/yolo_infer.hpp`：
   ```cpp
   class YoloInfer {
   public:
     YoloInfer(const std::string& model_path, bool use_cuda);
     std::vector<Detection> Infer(const cv::Mat& image);
   };
   struct Detection {
     cv::Rect bbox;
     int class_id;
     float confidence;
   };
   ```
3. `src/yolo_infer.cpp` 实现：letterbox、推理、反 letterbox、NMS
4. 写单元测试，对一张静态图像执行 Infer，验证能检测到至少 1 个物体

**验收标准**：
- 单元测试通过
- 对 bus.jpg 检测到 persons / bus（至少 3 个 bbox）
- 推理时间（不含预处理）<20ms

---

### 🖥️ 任务 4.5：detection_node 接入 ROS

**目标**：创建 ROS 2 节点，订阅图像，调用 YoloInfer，发布 Detection2DArray。

**输入**：任务 4.4 + 3.7

**执行内容**：
1. `src/detection_node.cpp`：
   - 构造时从参数读取 model_path, conf_threshold, nms_threshold
   - 实例化 YoloInfer（CUDA）
   - 节点启动时做一次 warm-up：传全黑 640×640 图
   - 订阅 `/camera/image_mono`，QoS=SensorDataQoS
   - 回调：cv_bridge 转 Mat → Infer → 构造 Detection2DArray → 发布 `/detections`
2. 创建 `config/detection.yaml`
3. 创建 `launch/detection.launch.py`

**验收标准**：
- `ros2 launch detection_node detection.launch.py` 启动正常
- `ros2 topic echo /detections` 能看到检测结果
- 用 Rviz2 的 Detection2DArray 插件（或 image_view 自己画框的辅助节点）能看到检测框

---

### 🖥️ 任务 4.6：编写检测结果可视化节点（辅助）

**目标**：一个小节点订阅图像和检测结果，在图上画框发出，方便 Rviz 查看。

**输入**：任务 4.5

**执行内容**：
1. 创建 `detection_viz_node`（可放在 detection_node 包内）
2. 订阅 `/camera/image_mono` 和 `/detections`，用 message_filters 同步
3. 在图像上画 bbox + 类别名 + 置信度
4. 发布 `/camera/image_detected`

**验收标准**：
- `rqt_image_view` 选 `/camera/image_detected` 能看到实时检测框
- 画面稳定，无明显错位

---

## 阶段 5：跟踪器

### 🖥️ 任务 5.1：集成 ByteTrack C++ 库

**目标**：把开源 ByteTrack 集成到项目里，独立跑通。

**输入**：任务 4.5 产生的 Detection2DArray

**执行内容**：
1. 从 GitHub 选一个轻量 ByteTrack C++ 实现（推荐搜索 "ByteTrack C++ Eigen only"）
2. 作为 git submodule 或直接复制到 `tracker_node/third_party/ByteTrack/`
3. 写一个独立 C++ 测试：硬编码几帧假的检测结果，喂进 ByteTrack，打印输出的 track ID

**验收标准**：
- 第一帧：两个 bbox 分配 ID=1, 2
- 第二帧：相近位置的 bbox 继续是 1, 2（不变）
- 完全不同位置的新 bbox：ID=3
- 一帧无检测后再出现：仍能匹配到原 ID（在 track_buffer 范围内）

---

### 🖥️ 任务 5.2：tracker_node 接入 ROS

**目标**：创建节点，订阅 `/detections`，输出带 ID 的 `/tracked_objects`。

**输入**：任务 5.1

**执行内容**：
1. `ros2 pkg create tracker_node`（依赖 rclcpp, vision_msgs）
2. 节点内部持有 BYTETracker 实例
3. 回调：
   - Detection2DArray 转 ByteTrack 输入格式
   - 调用 track()
   - 输出 track 结果填回 Detection2DArray（id 字段填 track_id）
   - 发布 `/tracked_objects`
4. 参数：track_thresh, match_thresh, track_buffer（放 YAML）

**验收标准**：
- 运行完整管道（gst → splitter → detection → tracker）
- `ros2 topic echo /tracked_objects` 每个 detection 的 id 字段非空
- 同一物体在画面中移动时，id 保持不变
- 物体被手短暂遮挡 1 秒后，id 仍保持

---

### 🖥️ 任务 5.3：扩展可视化节点显示 track ID

**目标**：在画框上额外显示 track ID 数字。

**输入**：任务 4.6 + 5.2

**执行内容**：
1. 修改 detection_viz_node 订阅 `/tracked_objects` 而非 `/detections`
2. 画框时标注 `ID:42 cup 0.87`

**验收标准**：
- `/camera/image_detected` 能看到每个框上的 track ID
- 同一物体 ID 数字稳定

---

## 阶段 6：决策与视觉伺服

### 🖥️ 任务 6.1：behavior_node 基础版（打印目标）

**目标**：订阅 `/tracked_objects`，根据固定类别（先硬编码 "person"）选目标并打印像素坐标。

**输入**：任务 5.2

**执行内容**：
1. `ros2 pkg create behavior_node`
2. 订阅 `/tracked_objects`
3. 过滤出类别 = "person" 的框，选 bbox 面积最大的
4. 打印 "Target: id=X, center=(cx, cy)"

**验收标准**：
- 人出现在画面 → 持续打印目标坐标
- 多个人 → 锁定最大那个
- 无人 → 打印 "No target"

---

### 🖥️ 任务 6.2：behavior_node 发布 /pixel_error

**目标**：把目标中心坐标转成相对画面中心的误差，发布为 `Vector3`。

**输入**：任务 6.1

**执行内容**：
1. 加 Publisher `/pixel_error` (geometry_msgs/Vector3)
2. error.x = target_cx - 160 （画面宽 320 的中心）
3. error.y = target_cy - 240 （画面高 480 的中心）
4. error.z = 0
5. 无目标时发 `(0, 0, 0)` 或不发（选后者）

**验收标准**：
- `ros2 topic echo /pixel_error` 持续显示误差
- 人从画面中心移到右边 → error.x 变正
- 人从中心移到上边 → error.y 变负

---

### 🖥️ 任务 6.3：添加 /set_target_class 服务

**目标**：支持运行时切换目标类别。

**输入**：任务 6.2 + robot_interfaces

**执行内容**：
1. behavior_node 注册 service `/set_target_class`
2. 节点内部维护 `current_target_class` 成员变量（默认 "person"）
3. service 回调：更新变量，返回 success=true
4. 目标选择逻辑改为根据 `current_target_class` 过滤

**验收标准**：
- `ros2 service call /set_target_class robot_interfaces/srv/SetTargetClass "{class_name: 'cup'}"` 返回成功
- 之后切换为只跟随杯子

---

### 🖥️ 任务 6.4：visual_servo_node 开环版

**目标**：收到 `/pixel_error`，不做 PID，只做简单比例转角度，发 `/servo_cmd`。

**输入**：任务 6.2

**执行内容**：
1. `ros2 pkg create visual_servo`
2. 订阅 `/pixel_error` 和 `/servo_state`
3. 维护 current_yaw, current_pitch（从 /servo_state 更新）
4. 30Hz 定时器：
   ```
   new_yaw = current_yaw - error.x * 0.05   # 向右偏就左转
   new_pitch = current_pitch - error.y * 0.04
   clamp 到 [10, 170]
   发布 /servo_cmd
   ```
5. 加死区：|error| < 5 像素不动

**验收标准**：
- 运行完整管道
- 人在画面中移动 → 舵机跟着转
- 但会有振荡/过冲，这很正常，下一步 PID 才解决

---

### 🖥️ 任务 6.5：visual_servo_node 完整 PID 版

**目标**：把任务 6.4 的纯 P 控制替换成完整 PID。

**输入**：任务 6.4

**执行内容**：
1. 为 yaw 和 pitch 各实现一个 PID 类：
   ```cpp
   class PID {
     float Kp, Ki, Kd, deadband, output_limit;
     float integral, prev_error;
   public:
     float Update(float error, float dt);
     void Reset();
   };
   ```
2. 从 `config/visual_servo.yaml` 加载所有参数
3. 每次 visual_servo 定时器调用 PID.Update()，输出作为角度增量
4. 加输出限幅 max_step_deg
5. 加集成饱和保护（clamp integral）

**验收标准**：
- 先只启 Kp：舵机跟随物体，但有明显过冲
- 加 Kd：过冲明显减小
- 加小 Ki：稳态误差减小
- 用 `rqt_plot /pixel_error/x` 能看到误差曲线收敛

---

## 阶段 7：集成与调参

### 📝 任务 7.1：编写完整 launch 文件 vision_stack.launch.py

**目标**：一条命令启动 WSL2 所有节点。

**输入**：阶段 3~6 完成

**执行内容**：
1. 创建 `robot_bringup` 包
2. `launch/vision_stack.launch.py`：启动 gst_receiver、stereo_splitter、detection_node、tracker_node、behavior_node、visual_servo_node
3. 每个节点 include 自己的 launch + YAML

**验收标准**：
- `ros2 launch robot_bringup vision_stack.launch.py` 一次性启动所有节点
- 每个节点都在日志中有启动信息
- Ctrl+C 能全部干净退出

---

### 📝 任务 7.2：编写 rpi_stack.launch.py

**目标**：一条命令启动树莓派所有节点。

**输入**：任务 2.7 完成

**执行内容**：
1. 创建 `launch/rpi_stack.launch.py`
2. 启动 uart_bridge_node
3. （暂不含 imu_node）

**验收标准**：
- 树莓派上 `ros2 launch robot_bringup rpi_stack.launch.py` 成功

---

### 🖥️ 任务 7.3：端到端联调 —— 静止物体

**目标**：把一切连起来，对静止物体能稳定锁定并指向。

**输入**：7.1 + 7.2

**执行内容**：
1. 启动：树莓派 start_camera_stream.sh、rpi_stack.launch.py；WSL2 vision_stack.launch.py
2. 确认 `ROS_DOMAIN_ID` 在两边一致
3. 桌上放一个杯子
4. 切换目标类别为 cup
5. 观察舵机是否指向杯子

**验收标准**：
- 从任意初始角度开始，舵机最终稳定指向杯子
- 误差 `/pixel_error` 收敛到死区内（<5 像素）
- 无持续振荡

---

### 🖥️ 任务 7.4：端到端联调 —— 缓慢移动物体

**目标**：手持物体缓慢移动（10 cm/s），舵机平滑跟随。

**输入**：7.3

**执行内容**：
1. 手持杯子从画面一侧缓慢移到另一侧
2. 观察舵机响应
3. 用 `rqt_plot /pixel_error/x /pixel_error/y` 录制曲线
4. 根据曲线调 PID：
   - 有振荡 → 减 Kp 或增 Kd
   - 响应慢 → 增 Kp
   - 稳态有偏 → 增 Ki（小）

**验收标准**：
- 物体在画面中始终保持在中心附近（±30 像素）
- 舵机动作平滑无抖
- pixel_error 曲线无明显振荡

---

### 🖥️ 任务 7.5：端到端联调 —— 遮挡恢复

**目标**：验证 ByteTrack 抗遮挡能力。

**输入**：7.4

**执行内容**：
1. 物体跟踪中，用手短暂遮挡目标 1~2 秒
2. 观察 ID 是否保持

**验收标准**：
- 遮挡期间：舵机保持最后已知位置
- 遮挡结束：同一物体 track_id 不变，继续跟踪

---

### 📝 任务 7.6：编写 docs/pid_tuning.md 记录调参结果

**目标**：把调好的参数和调参过程记录下来，未来换硬件或场景时能复用。

**输入**：7.4 调好的 YAML

**执行内容**：
1. 记录最终 Kp/Ki/Kd 值
2. 记录调参过程（从什么值开始，改了什么，为什么）
3. 贴几张 rqt_plot 的曲线截图
4. 记录已知问题（如 SG90 固有死区）

**验收标准**：
- 文档在 `docs/pid_tuning.md`
- 另一个人读完能独立重现你的调参过程

---

## 阶段完成检查表

- [ ] 阶段 0：0.1 → 0.2 → 0.3
- [ ] 阶段 1：1.1 → 1.2 → 1.3 → 1.4 → 1.5
- [ ] 阶段 2：2.1 → 2.2 → 2.3 → 2.4 → 2.5 → 2.6 → 2.7
- [ ] 阶段 3：3.1 → 3.2 → 3.3 → 3.4 → 3.5 → 3.6 → 3.7
- [ ] 阶段 4：4.1 → 4.2 → 4.3 → 4.4 → 4.5 → 4.6
- [ ] 阶段 5：5.1 → 5.2 → 5.3
- [ ] 阶段 6：6.1 → 6.2 → 6.3 → 6.4 → 6.5
- [ ] 阶段 7：7.1 → 7.2 → 7.3 → 7.4 → 7.5 → 7.6

**总任务数：40 个**

全部完成后你将拥有一个功能完整的"物体识别 + 舵机跟踪"桌面机器人。后续扩展（IMU、深度、语音）可在此基础上增量开发。

---

**文档结束**
