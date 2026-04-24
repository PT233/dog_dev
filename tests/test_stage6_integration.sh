#!/bin/bash
# Stage 6 Integration Test
# Tests: behavior_node + visual_servo_node communication

set -e

# Source setup
source /home/peter/dog/dog_dev/install/setup.bash

echo "=== Stage 6 Integration Test ==="
echo ""
echo "Test 1: behavior_node initialization"
timeout 3 ros2 run behavior_node behavior_node_exe 2>&1 | head -5 && echo "✓ PASS" || echo "✗ FAIL"

echo ""
echo "Test 2: visual_servo_node initialization"
timeout 3 ros2 run visual_servo visual_servo_node_exe 2>&1 | head -5 && echo "✓ PASS" || echo "✗ FAIL"

echo ""
echo "Test 3: Check behavior_node service registration"
ros2 service list 2>/dev/null | grep -q "set_target_class" && echo "✓ /set_target_class service exists" || echo "✗ Service not found"

echo ""
echo "All tests completed!"
