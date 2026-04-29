#!/bin/bash

# 任务 7.3：静止物体追踪端到端测试
# 本脚本提供完整的操作步骤和命令

cat << 'EOF'

╔════════════════════════════════════════════════════════════════╗
║            任务 7.3：端到端联调 —— 静止物体追踪               ║
╚════════════════════════════════════════════════════════════════╝

[前置要求]
✓ STM32 固件已烧录
✓ 硬件接线完整（树莓派 UART ↔ STM32）
✓ 树莓派和 PC 可以 SSH 连接
✓ ROS 2 Jazzy 已在两端安装
✓ 准备一个杯子放在摄像机视野内

[测试场景]
- 在桌上放置一个杯子
- 系统需要自动追踪杯子
- 舵机应该指向杯子中心
- 像素误差应该收敛到 <5px 的死区内

════════════════════════════════════════════════════════════════

[第一步] 设置环境变量（两端都需要）

在 WSL2 中：
  cd /home/peter/dog/dog_dev
  export ROBOT_DDS_ROLE=wsl
  source scripts/ros2_network_env.sh

在树莓派中：
  ssh ubuntu@192.168.137.100
  cd ~/desktop_tracking_robot
  export ROBOT_DDS_ROLE=rpi
  export ROBOT_WSL_IP=<当前 WSL2 IP>
  source scripts/ros2_network_env.sh

════════════════════════════════════════════════════════════════

[第二步] 启动系统（需要 4 个终端）

📱 Terminal 1 - 树莓派：启动相机推流
  ssh ubuntu@192.168.137.100
  cd ~/desktop_tracking_robot  # 或项目目录
  export ROBOT_DDS_ROLE=rpi
  source scripts/ros2_network_env.sh
  ./scripts/start_camera_stream.sh

  预期输出：
    [GStreamer] Pipeline running...
    [V4L2] Reading from /dev/video0

📱 Terminal 2 - 树莓派：启动树莓派 ROS 节点
  ssh ubuntu@192.168.137.100
  source ~/ros2_ws/install/setup.bash
  cd ~/desktop_tracking_robot
  export ROBOT_DDS_ROLE=rpi
  source scripts/ros2_network_env.sh
  ros2 launch robot_bringup rpi_stack.launch.py

  预期输出：
    [uart_bridge_node] Initialized
    [uart_bridge_node] Subscribed to /leg_motion_node/output/servo_command

📱 Terminal 3 - WSL2：启动视觉管道
  cd /home/peter/dog/dog_dev
  source install/setup.bash
  export ROBOT_DDS_ROLE=wsl
  source scripts/ros2_network_env.sh
  ros2 launch robot_bringup vision_stack.launch.py

  预期输出：
    [gst_receiver_node] publishing to /stereo/image_raw
    [detection_node] model loaded
    [tracker_node] initialized
    [behavior_node] started
    [leg_motion_node] started

════════════════════════════════════════════════════════════════

[第三步] 切换目标类别为杯子

在 Terminal 4（新窗口）中运行：
  cd /home/peter/dog/dog_dev
  export ROBOT_DDS_ROLE=wsl
  source scripts/ros2_network_env.sh
  ros2 service call /behavior_node/input/set_target_class robot_interfaces/srv/SetTargetClass "{class_name: 'cup'}"

预期输出：
  requester: making request #1: robot_interfaces.srv.SetTargetClass_Request(class_name='cup')
  response: robot_interfaces.srv.SetTargetClass_Response(success=true)

════════════════════════════════════════════════════════════════

[第四步] 验证追踪效果（需要杯子摆放在相机视野内）

📊 在 Terminal 4 中运行自动验证脚本：
  cd /home/peter/dog/dog_dev
  bash scripts/verify_tracking.sh

脚本会自动检查：
  ✓ 所有节点是否运行
  ✓ 话题频率是否正常（~30 Hz）
  ✓ 像素误差是否收敛到 <5px
  ✓ 无持续振荡

════════════════════════════════════════════════════════════════

[验收标准]

✓ 能够检测到杯子（/detection_node/output/detections 有输出）
✓ 能够追踪杯子（/tracker_node/output/tracked_objects 的 track_id 保持一致）
✓ 像素误差收敛到死区内（<5 像素）
✓ 舵机响应平滑，无持续振荡

[常见问题]

Q: 看不到杯子？
A: 检查：
  1. 相机视野是否包含杯子
  2. 光线是否充足
  3. 杯子颜色是否足够鲜明
  4. 运行 rqt_image_view 查看摄像机输出

Q: 舵机不动？
A: 检查：
  1. /leg_motion_node/output/servo_command 话题是否有输出
  2. STM32 UART 是否连接
  3. 树莓派的 /dev/ttyAMA0 是否有权限
  4. 运行 ros2 topic echo /leg_motion_node/output/servo_command 查看命令

Q: 误差不收敛？
A: 检查：
  1. 目标类别是否正确（运行 ros2 topic echo /behavior_node/output/pixel_error）
  2. PID 参数是否合适（见 config/visual_servo.yaml）
  3. 舵机机械是否有死区（SG90 通常有 ±5px 死区）

════════════════════════════════════════════════════════════════

[调试命令]

监控像素误差：
  ros2 topic echo /behavior_node/output/pixel_error

监控追踪对象：
  ros2 topic echo /tracker_node/output/tracked_objects

实时绘制误差曲线：
  rqt_plot /behavior_node/output/pixel_error/x /behavior_node/output/pixel_error/y

查看摄像机输出：
  rqt_image_view

列出所有话题：
  ros2 topic list

检查节点状态：
  ros2 node list

════════════════════════════════════════════════════════════════

[关闭系统]

按照反向顺序关闭（按 Ctrl+C）：
  Terminal 3: Ctrl+C （停止 WSL2 节点）
  Terminal 2: Ctrl+C （停止树莓派节点）
  Terminal 1: Ctrl+C （停止相机推流）

验证所有进程已停止：
  pkill -f "ros2 launch"
  pkill -f "gst-launch"

════════════════════════════════════════════════════════════════

[下一步]

测试成功后（误差收敛 <5px），进行任务 7.4：
- 缓慢移动杯子，观察舵机跟踪
- 用 rqt_plot 录制误差曲线
- 根据曲线调整 PID 参数

════════════════════════════════════════════════════════════════

EOF
