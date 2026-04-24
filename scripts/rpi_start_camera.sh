#!/bin/bash

# 树莓派端：启动相机推流
# 该脚本会自动寻找正确的配置和脚本位置

set -e

echo "=========================================="
echo "树莓派相机推流启动脚本"
echo "=========================================="
echo ""

# 检测项目目录
PROJECT_DIR=""
if [ -d ~/dog/dog_dev ]; then
    PROJECT_DIR=~/dog/dog_dev
elif [ -d ~/desktop_tracking_robot ]; then
    PROJECT_DIR=~/desktop_tracking_robot
elif [ -d ~/ros2_ws ]; then
    PROJECT_DIR=~/
else
    echo "❌ 错误：找不到项目目录"
    echo "请确保以下任一目录存在："
    echo "  ~/dog/dog_dev"
    echo "  ~/desktop_tracking_robot"
    echo "  ~/ros2_ws"
    exit 1
fi

echo "✓ 项目目录：$PROJECT_DIR"
echo ""

# 设置环境变量
export ROS_DOMAIN_ID=42
export ROS_LOCALHOST_ONLY=0

echo "ROS_DOMAIN_ID = $ROS_DOMAIN_ID"
echo ""

# 启动相机推流（使用 GStreamer）
# 树莓派摄像头 → YUYV 640×480 @ 30fps → H.264 编码 → UDP 推送

echo "启动相机推流..."
echo "目标地址：192.168.137.1:5600"
echo ""

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

echo ""
echo "❌ 相机推流已停止"
