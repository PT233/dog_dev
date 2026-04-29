#!/bin/bash

# 任务 7.3 验证脚本：自动检查静止物体追踪的各项指标

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
if [ -f "$PROJECT_DIR/scripts/ros2_network_env.sh" ]; then
    export ROBOT_DDS_ROLE="${ROBOT_DDS_ROLE:-wsl}"
    source "$PROJECT_DIR/scripts/ros2_network_env.sh"
fi

RESET='\033[0m'
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'

echo -e "${BLUE}════════════════════════════════════════════════════════════════${RESET}"
echo -e "${BLUE}        任务 7.3 验证脚本：静止物体追踪端到端测试${RESET}"
echo -e "${BLUE}════════════════════════════════════════════════════════════════${RESET}"
echo ""

# 确保设置了 ROS_DOMAIN_ID
if [ -z "$ROS_DOMAIN_ID" ]; then
    echo -e "${YELLOW}⚠ 警告：ROS_DOMAIN_ID 未设置${RESET}"
    echo "   运行以下命令："
    echo "   export ROS_DOMAIN_ID=42"
    exit 1
fi

echo -e "${CYAN}[1/5] 检查 ROS 节点...${RESET}"
echo ""

NODES=$(ros2 node list 2>/dev/null || echo "")
if [ -z "$NODES" ]; then
    echo -e "${RED}✗ 无法列出节点（可能没有启动 ROS）${RESET}"
    exit 1
fi

echo "$NODES" | head -10
echo ""

# 检查关键节点
REQUIRED_NODES=(
    "/gst_receiver_node"
    "/detection_node"
    "/tracker_node"
    "/behavior_node"
    "/leg_motion_node"
    "/uart_bridge_node"
)

for node in "${REQUIRED_NODES[@]}"; do
    if echo "$NODES" | grep -q "$node"; then
        echo -e "${GREEN}✓${RESET} $node"
    else
        echo -e "${RED}✗${RESET} $node"
    fi
done
echo ""

echo -e "${CYAN}[2/5] 检查话题频率...${RESET}"
echo ""

# 检查关键话题频率（采样 3 秒）。
# 这里使用全局 topic 名是为了让 ros2 CLI 校验运行时已解析的完整图名，便于跨节点端到端检查。
TOPICS_TO_CHECK=(
    "/gst_receiver_node/output/stereo_image_raw:~30"
    "/tracker_node/output/tracked_objects:~30"
    "/behavior_node/output/pixel_error:~30"
    "/leg_motion_node/output/servo_command:~30"
)

for topic_freq in "${TOPICS_TO_CHECK[@]}"; do
    topic="${topic_freq%:*}"
    expected_freq="${topic_freq#*:}"

    echo -ne "  $topic... "
    freq=$(timeout 3 ros2 topic hz "$topic" 2>/dev/null | grep "average frequency" | awk '{print $NF}' || echo "0")

    if (( $(echo "$freq > 20" | bc -l) )); then
        echo -e "${GREEN}$freq Hz ✓${RESET}"
    else
        echo -e "${YELLOW}$freq Hz ⚠${RESET}"
    fi
done
echo ""

echo -e "${CYAN}[3/5] 检查 /tracker_node/output/tracked_objects 输出...${RESET}"
echo ""

detection=$(timeout 2 ros2 topic echo /tracker_node/output/tracked_objects --limit 1 2>/dev/null || echo "")
if [ -z "$detection" ]; then
    echo -e "${YELLOW}⚠ 未收到 /tracker_node/output/tracked_objects 数据${RESET}"
    echo "   可能原因："
    echo "   - 摄像机中没有检测到物体"
    echo "   - 目标类别不匹配"
    echo "   - 检测节点未启动"
else
    echo -e "${GREEN}✓ 收到追踪数据${RESET}"
    echo "$detection" | head -5
fi
echo ""

echo -e "${CYAN}[4/5] 采样像素误差（持续 10 秒）...${RESET}"
echo ""

# 采样像素误差
timeout 10 ros2 topic echo /behavior_node/output/pixel_error 2>/dev/null | grep -A 2 "x:" | head -30 > /tmp/pixel_errors.txt

if [ -s /tmp/pixel_errors.txt ]; then
    echo -e "${GREEN}✓ 成功采集误差数据${RESET}"
    echo ""

    # 提取最后几个误差值
    echo "  最近的像素误差样本："
    tail -9 /tmp/pixel_errors.txt | grep -E "^\s+(x|y):" | sed 's/^/    /'
else
    echo -e "${YELLOW}⚠ 未收到 /behavior_node/output/pixel_error 数据${RESET}"
    echo "   可能原因："
    echo "   - behavior_node 未启动"
    echo "   - 没有检测到目标"
fi
echo ""

echo -e "${CYAN}[5/5] 验收标准检查...${RESET}"
echo ""

# 解析最后一个误差值
LAST_ERROR=$(tail -2 /tmp/pixel_errors.txt | grep "x:" | tail -1 | grep -oP '(?<=x:\s)\d+' || echo "999")

echo "  最终像素误差 (x): $LAST_ERROR pixels"

if [ "$LAST_ERROR" != "999" ]; then
    if (( LAST_ERROR < 5 )); then
        echo -e "${GREEN}✓ 误差 < 5px（死区内）${RESET}"
        echo -e "${GREEN}✓ 任务 7.3 验收通过${RESET}"
    elif (( LAST_ERROR < 10 )); then
        echo -e "${YELLOW}⚠ 误差 5~10px（接近死区）${RESET}"
        echo -e "${YELLOW}  可能需要调整 PID 参数${RESET}"
    else
        echo -e "${RED}✗ 误差 > 10px（超出预期）${RESET}"
        echo -e "${RED}  需要检查舵机响应或 PID 参数${RESET}"
    fi
else
    echo -e "${YELLOW}⚠ 无法获取误差数据${RESET}"
fi

echo ""
echo -e "${BLUE}════════════════════════════════════════════════════════════════${RESET}"
echo ""

# 打印调试命令
echo -e "${YELLOW}[调试命令参考]${RESET}"
echo ""
echo "  监控像素误差（实时）："
echo "    ros2 topic echo /behavior_node/output/pixel_error"
echo ""
echo "  查看追踪对象："
echo "    ros2 topic echo /tracker_node/output/tracked_objects"
echo ""
echo "  实时绘制误差曲线："
echo "    rqt_plot /behavior_node/output/pixel_error/x /behavior_node/output/pixel_error/y &"
echo ""
echo "  查看摄像机输出："
echo "    rqt_image_view"
echo ""
echo "  列出所有节点："
echo "    ros2 node list"
echo ""
