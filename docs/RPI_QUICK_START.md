# 树莓派快速启动指南

> 此指南适用于树莓派端的启动步骤

## 前置条件

1. **ROS 2 Jazzy 已安装**
   ```bash
   which ros2
   # 输出：/opt/ros/jazzy/bin/ros2
   ```

2. **ros2_ws 已编译**
   ```bash
   ls ~/ros2_ws/install/setup.bash
   # 应该存在此文件
   ```

3. **UART 连接正确**
   ```bash
   ls -l /dev/ttyAMA0
   # 应该有权限读写
   ```

---

## 快速启动 (3 个命令)

### 方式 1：使用提供的脚本（推荐）

将以下脚本从 PC 复制到树莓派：

```bash
# 在树莓派上执行
scp -r pc_user@192.168.137.1:/path/to/scripts/rpi_*.sh ~/

# 或者手动创建：
# 见下方"手动创建脚本"部分
```

然后：

```bash
# Terminal 1：启动相机推流
bash ~/rpi_start_camera.sh

# Terminal 2：启动 ROS 节点
bash ~/rpi_start_ros.sh
```

### 方式 2：直接运行命令

**Terminal 1：相机推流**

```bash
export ROS_DOMAIN_ID=42
export ROS_LOCALHOST_ONLY=0

gst-launch-1.0 -v \
  v4l2src device=/dev/video0 ! \
  "video/x-raw,format=YUYV,width=640,height=480,framerate=30/1" ! \
  videoconvert ! \
  v4l2h264enc extra-controls="controls,h264_level=11,h264_profile=0" ! \
  "video/x-h264,profile=baseline" ! \
  h264parse ! \
  rtph264pay pt=96 ! \
  "application/x-rtp,media=video,encoding-name=H264,payload=96" ! \
  udpsink host=192.168.137.1 port=5600 sync=false async=false
```

**Terminal 2：ROS 节点**

```bash
export ROS_DOMAIN_ID=42
export ROS_LOCALHOST_ONLY=0
source ~/ros2_ws/install/setup.bash

ros2 launch robot_bringup rpi_stack.launch.py
```

---

## 预期输出

### Terminal 1（相机推流）

```
Setting pipeline to PAUSED ...
Pipeline is PAUSED ...
Setting pipeline to PLAYING ...
New clock: GstSystemClock
/GstPipeline:pipeline0/GstV4l2Src:v4l2src0.GstPad:src: caps = video/x-raw, format=(string)YUYV, width=(int)640, height=(int)480, ...
...
```

### Terminal 2（ROS 节点）

```
[uart_bridge_node-1] [INFO] [1714022400.123456]: uart_bridge_node started
[uart_bridge_node-1] [INFO] [1714022400.234567]: Subscribed to /servo_cmd
[uart_bridge_node-1] [INFO] [1714022400.345678]: Publishing /servo_state
...
```

---

## 常见问题

### Q: 找不到 ros2_ws？

```bash
# 检查目录
ls ~/ | grep ros2
ls ~/ | grep dog

# 如果没有，需要从 PC 复制
scp -r pc_user@192.168.137.1:/home/peter/dog/dog_dev/ros2_ws ~/
```

### Q: UART 权限不足？

```bash
# 添加权限
sudo usermod -aG dialout ubuntu
# 需要重新登录或重启
```

### Q: 摄像头找不到？

```bash
# 检查摄像头
ls -l /dev/video*

# 列出摄像头信息
v4l2-ctl --list-devices
```

### Q: GStreamer 报错？

```bash
# 检查安装
which gst-launch-1.0

# 列出可用的编码器
gst-inspect-1.0 | grep h264
```

---

## 手动创建脚本

如果无法复制脚本，可以在树莓派上手动创建：

**创建 ~/start_camera.sh**

```bash
cat > ~/start_camera.sh << 'EOF'
#!/bin/bash
export ROS_DOMAIN_ID=42
export ROS_LOCALHOST_ONLY=0
gst-launch-1.0 -v \
  v4l2src device=/dev/video0 ! \
  "video/x-raw,format=YUYV,width=640,height=480,framerate=30/1" ! \
  videoconvert ! \
  v4l2h264enc extra-controls="controls,h264_level=11,h264_profile=0" ! \
  "video/x-h264,profile=baseline" ! \
  h264parse ! \
  rtph264pay pt=96 ! \
  "application/x-rtp,media=video,encoding-name=H264,payload=96" ! \
  udpsink host=192.168.137.1 port=5600 sync=false async=false
EOF

chmod +x ~/start_camera.sh
```

**创建 ~/start_ros.sh**

```bash
cat > ~/start_ros.sh << 'EOF'
#!/bin/bash
export ROS_DOMAIN_ID=42
export ROS_LOCALHOST_ONLY=0
source ~/ros2_ws/install/setup.bash
ros2 launch robot_bringup rpi_stack.launch.py
EOF

chmod +x ~/start_ros.sh
```

然后运行：

```bash
bash ~/start_camera.sh
bash ~/start_ros.sh
```

---

## 关闭系统

按照以下顺序关闭（反向启动顺序）：

1. Terminal 2（ROS 节点）：按 `Ctrl+C`
2. Terminal 1（相机推流）：按 `Ctrl+C`

验证所有进程已停止：

```bash
pkill -f gst-launch
pkill -f ros2
```

---

## 下一步

系统启动成功后：

1. **WSL2 端启动视觉管道** (vision_stack.launch.py)
2. **切换目标类别为杯子**
3. **运行验证脚本** (verify_tracking.sh)

详见 `docs/task_7_3_guide.md`

---

**Last Updated**: 2026-04-24
