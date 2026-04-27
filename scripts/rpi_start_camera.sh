#!/usr/bin/env bash

# 树莓派端：启动相机推流
# 该脚本会自动寻找正确的配置和脚本位置

set -euo pipefail

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

TARGET_IP=${1:-192.168.137.1}
TARGET_PORT=${2:-5600}

echo "启动相机推流..."
echo "目标地址：$TARGET_IP:$TARGET_PORT"
echo ""

"$PROJECT_DIR/scripts/start_camera_stream.sh" "$TARGET_IP" "$TARGET_PORT"

echo ""
echo "❌ 相机推流已停止"
