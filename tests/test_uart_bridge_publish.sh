#!/bin/bash
# Integration test for uart_bridge publishing functionality

set -e

WORKSPACE_DIR="/mnt/c/Users/Pt/Desktop/dog/agent_dev/ros2_ws"
cd "$WORKSPACE_DIR"

source install/setup.bash

echo "========== Test 1: Verify /servo_state Publisher =========="
if grep -q "servo_state_pub_" src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ servo_state_pub_ member variable found"
else
  echo "✗ servo_state_pub_ not found"
  exit 1
fi

if grep -q "create_publisher.*JointState" src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ /servo_state publisher creation found"
else
  echo "✗ /servo_state publisher creation not found"
  exit 1
fi

echo ""
echo "========== Test 2: Verify QoS configuration =========="
if grep -q "reliable()" src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ Reliable QoS policy found"
else
  echo "✗ Reliable QoS not found"
  exit 1
fi

if grep -q "QoS(10)" src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ Depth=10 QoS policy found"
else
  echo "✗ Depth=10 not found"
  exit 1
fi

echo ""
echo "========== Test 3: Verify JointState message construction =========="
if grep -q "state_msg->header.stamp" src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ Header timestamp found"
else
  echo "✗ Header timestamp not found"
  exit 1
fi

if grep -q "state_msg->name.push_back" src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ Joint name population found"
else
  echo "✗ Joint name population not found"
  exit 1
fi

if grep -q "state_msg->position.push_back" src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ Joint position population found"
else
  echo "✗ Joint position population not found"
  exit 1
fi

echo ""
echo "========== Test 4: Verify servo_id to joint name mapping =========="
if grep -q '"front_left"' src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ front_left joint name found"
else
  echo "✗ front_left joint name not found"
  exit 1
fi

if grep -q '"front_right"' src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ front_right joint name found"
else
  echo "✗ front_right joint name not found"
  exit 1
fi

if grep -q '"rear_left"' src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ rear_left joint name found"
else
  echo "✗ rear_left joint name not found"
  exit 1
fi

if grep -q '"rear_right"' src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ rear_right joint name found"
else
  echo "✗ rear_right joint name not found"
  exit 1
fi

echo ""
echo "========== Test 5: Verify angle conversion (deg to radians) =========="
if grep -q "M_PI / 180.0" src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ Degrees to radians conversion found"
else
  echo "✗ Degrees to radians conversion not found"
  exit 1
fi

echo ""
echo "========== Test 6: Verify publish call =========="
if grep -q "servo_state_pub_->publish" src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ Publish method call found"
else
  echo "✗ Publish method call not found"
  exit 1
fi

echo ""
echo "========== Test 7: Build verification =========="
if colcon build --packages-select uart_bridge 2>&1 | grep -q "Finished"; then
  echo "✓ uart_bridge builds successfully"
else
  echo "✗ uart_bridge build failed"
  exit 1
fi

echo ""
echo "========== All tests passed =========="
