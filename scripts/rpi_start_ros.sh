#!/usr/bin/env bash

# 树莓派端：启动 ROS 2 节点 (uart_bridge_node)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "=========================================="
echo "树莓派 ROS 2 节点启动脚本"
echo "=========================================="
echo ""

# 设置环境变量
export ROS_DOMAIN_ID=42
export ROBOT_DDS_ROLE="${ROBOT_DDS_ROLE:-rpi}"
if [ -f "$SCRIPT_DIR/ros2_network_env.sh" ]; then
    source "$SCRIPT_DIR/ros2_network_env.sh"
else
    export ROS_LOCALHOST_ONLY=0
fi

echo "ROS_DOMAIN_ID = $ROS_DOMAIN_ID"
echo "RMW_IMPLEMENTATION = ${RMW_IMPLEMENTATION:-}"
echo "CYCLONEDDS_URI = ${CYCLONEDDS_URI:-}"
echo ""

# 查找 ros2_ws
ROS_WS_DIR=""
if [ -d ~/ros2_ws ]; then
    ROS_WS_DIR=~/ros2_ws
elif [ -d ~/dog/dog_dev/ros2_ws ]; then
    ROS_WS_DIR=~/dog/dog_dev/ros2_ws
else
    echo "❌ 错误：找不到 ros2_ws 目录"
    echo "请确保 ros2_ws 存在或已编译"
    exit 1
fi

echo "✓ ROS workspace：$ROS_WS_DIR"
echo ""

# Source ROS setup
if [ -f /opt/ros/jazzy/setup.bash ]; then
    set +u
    source /opt/ros/jazzy/setup.bash
    set -u
else
    echo "❌ 错误：未找到 /opt/ros/jazzy/setup.bash"
    exit 1
fi

if [ -f "$ROS_WS_DIR/install/setup.bash" ]; then
    echo "加载 ROS 环境..."
    set +u
    source "$ROS_WS_DIR/install/setup.bash"
    set -u
else
    echo "⚠️  警告：未找到 install/setup.bash"
    echo "尝试使用已编译的包..."
fi

echo ""
echo "启动 uart_bridge_node..."
echo ""

# 启动 uart_bridge 节点
# 此节点负责：
# 1. 监听 /leg_motion_node/output/servo_command 话题（来自 leg_motion_node）
# 2. 通过 UART 发送舵机控制命令给 STM32
# 3. 发布 /uart_bridge_node/output/servo_state 话题（STM32 的状态反馈）

ros2 launch robot_bringup rpi_stack.launch.py

echo ""
echo "❌ ROS 节点已停止"
