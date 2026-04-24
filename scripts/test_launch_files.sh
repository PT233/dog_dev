#!/bin/bash
# Test launch files availability

set -e

echo "=== Testing Launch Files ==="
echo ""

# Source ROS 2 setup
source /home/peter/dog/dog_dev/install/setup.bash

echo "Test 1: Check robot_bringup package"
ros2 pkg prefix robot_bringup && echo "✓ robot_bringup found" || echo "✗ robot_bringup not found"

echo ""
echo "Test 2: Check vision_stack.launch.py syntax"
python3 -m py_compile /home/peter/dog/dog_dev/ros2_ws/src/robot_bringup/launch/vision_stack.launch.py && echo "✓ vision_stack.launch.py syntax OK" || echo "✗ Syntax error"

echo ""
echo "Test 3: Check rpi_stack.launch.py syntax"
python3 -m py_compile /home/peter/dog/dog_dev/ros2_ws/src/robot_bringup/launch/rpi_stack.launch.py && echo "✓ rpi_stack.launch.py syntax OK" || echo "✗ Syntax error"

echo ""
echo "Test 4: List launch files"
ros2 launch robot_bringup --list 2>&1 | grep -E "vision_stack|rpi_stack" && echo "✓ Launch files listed" || echo "✗ Launch files not found"

echo ""
echo "All tests completed!"
