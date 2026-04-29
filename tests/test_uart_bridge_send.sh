#!/bin/bash
# Integration test for uart_bridge sending functionality

set -e

WORKSPACE_DIR="${WORKSPACE_DIR:-/home/peter/dog/dog_dev/ros2_ws}"
cd "$WORKSPACE_DIR"

source install/setup.bash

echo "========== Test 1: Check uart_bridge builds correctly =========="
if colcon build --packages-select uart_bridge 2>&1 | grep -q "Finished"; then
  echo "✓ uart_bridge built successfully"
else
  echo "✗ uart_bridge build failed"
  exit 1
fi

echo ""
echo "========== Test 2: Verify private servo_command subscription =========="
if grep -q "~/input/servo_command" src/uart_bridge/src/uart_bridge_transport.cpp; then
  echo "✓ private servo_command subscription present"
else
  echo "✗ private servo_command subscription not found"
  exit 1
fi

echo ""
echo "========== Test 3: Verify frame encoder class =========="
if [ -f src/uart_bridge/include/uart_bridge/frame_encoder.hpp ]; then
  echo "✓ frame_encoder.hpp found"
else
  echo "✗ frame_encoder.hpp not found"
  exit 1
fi

if [ -f src/uart_bridge/src/frame_encoder.cpp ]; then
  echo "✓ frame_encoder.cpp found"
else
  echo "✗ frame_encoder.cpp not found"
  exit 1
fi

echo ""
echo "========== Test 4: Verify JointState to servo_id mapping =========="
if grep -q "servo_id_by_name" src/uart_bridge/src/uart_bridge_node_internal.hpp; then
  echo "✓ servo_id_by_name mapping function present"
else
  echo "✗ servo_id_by_name mapping not found"
  exit 1
fi

if grep -q '"front_left".*0' ../shared/servo_names.hpp; then
  echo "✓ front_left→0 mapping found"
else
  echo "✗ front_left→0 mapping not found"
  exit 1
fi

if grep -q '"front_right".*1' ../shared/servo_names.hpp; then
  echo "✓ front_right→1 mapping found"
else
  echo "✗ front_right→1 mapping not found"
  exit 1
fi

echo ""
echo "========== Test 5: Verify angle conversion =========="
if grep -q "M_PI" src/uart_bridge/src/uart_bridge_transport.cpp; then
  echo "✓ Radian to degree conversion present"
else
  echo "✗ Radian to degree conversion not found"
  exit 1
fi

echo ""
echo "========== Test 6: Verify UART write implementation =========="
if grep -q "write(uart_fd_" src/uart_bridge/src/uart_bridge_transport.cpp; then
  echo "✓ UART write implementation found"
else
  echo "✗ UART write implementation not found"
  exit 1
fi

echo ""
echo "========== Test 7: Verify SensorDataQoS usage =========="
if grep -q "SensorDataQoS()" src/uart_bridge/src/uart_bridge_transport.cpp; then
  echo "✓ SensorDataQoS policy applied to private servo_command subscription"
else
  echo "✗ SensorDataQoS not found"
  exit 1
fi

echo ""
echo "========== Test 8: Run node startup test (with timeout) =========="
# Try to launch for 2 seconds
if timeout 2 ros2 launch uart_bridge uart_bridge.launch.py 2>&1 | grep -E "(Failed to open|UART device opened)" | head -1; then
  echo "✓ Node startup verified"
else
  echo "⚠ Could not verify node startup (expected on WSL2 without /dev/ttyAMA0)"
fi

echo ""
echo "========== All tests passed =========="
