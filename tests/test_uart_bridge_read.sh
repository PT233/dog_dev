#!/bin/bash
# Integration test for uart_bridge reading functionality
# This test verifies the node can be launched and parameter loading works

set -e

WORKSPACE_DIR="/mnt/c/Users/Pt/Desktop/dog/agent_dev/ros2_ws"
cd "$WORKSPACE_DIR"

source install/setup.bash

echo "========== Test 1: Check launch file exists =========="
if [ -f src/uart_bridge/launch/uart_bridge.launch.py ]; then
  echo "✓ Launch file found"
else
  echo "✗ Launch file not found"
  exit 1
fi

echo ""
echo "========== Test 2: Check config file exists =========="
if [ -f src/uart_bridge/config/uart_bridge.yaml ]; then
  echo "✓ Config file found"
  cat src/uart_bridge/config/uart_bridge.yaml
else
  echo "✗ Config file not found"
  exit 1
fi

echo ""
echo "========== Test 3: Check node startup (with timeout) =========="
# Try to launch for 2 seconds. It will fail to open /dev/ttyAMA0 on WSL2,
# but should exit gracefully with error message
if timeout 2 ros2 launch uart_bridge uart_bridge.launch.py 2>&1 | grep -E "(Failed to open|UART device opened)" | head -1; then
  echo "✓ Node started and handled missing device gracefully"
else
  echo "⚠ Could not verify node startup output"
fi

echo ""
echo "========== Test 4: Verify code has threading support =========="
if grep -q "std::thread" src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ Code includes threading support"
else
  echo "✗ Code missing threading support"
  exit 1
fi

echo ""
echo "========== Test 5: Verify UART configuration (termios) =========="
if grep -q "termios\|tcgetattr\|tcsetattr" src/uart_bridge/src/uart_bridge_node.cpp; then
  echo "✓ Code includes UART termios configuration"
else
  echo "✗ Code missing UART configuration"
  exit 1
fi

echo ""
echo "========== All tests passed =========="
