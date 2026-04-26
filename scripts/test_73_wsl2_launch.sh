#!/bin/bash

# 任务 7.3 测试启动脚本 - WSL2 完整版
# 包含所有视觉节点 + 模拟的 uart_bridge

set -e

cd /home/peter/dog/dog_dev

echo "=========================================="
echo "任务 7.3：WSL2 完整启动脚本"
echo "=========================================="
echo ""

# 设置环境
export ROS_DOMAIN_ID=42
export ROS_LOCALHOST_ONLY=0

source install/setup.bash

echo "✓ ROS 环境已加载"
echo "✓ ROS_DOMAIN_ID = $ROS_DOMAIN_ID"
echo ""

echo "启动所有节点..."
echo ""

# 启动主要的视觉管道
# 注意：这里启动的是vision_stack（不包含uart_bridge）
# uart_bridge 会在树莓派上独立运行，或用模拟版本

ros2 launch robot_bringup vision_stack.launch.py &
VISION_PID=$!

# 等待所有节点启动
echo ""
echo "等待节点初始化（10 秒）..."
sleep 10

echo ""
echo "✓ 所有节点已启动！"
echo ""
echo "在另一个终端运行验证脚本："
echo "  cd /home/peter/dog/dog_dev"
echo "  bash scripts/verify_tracking.sh"
echo ""

# 保持前台运行
wait $VISION_PID
