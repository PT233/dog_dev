#!/bin/bash
# Test vision_stack launch file

set -e

source /home/peter/dog/dog_dev/install/setup.bash

echo "=== Testing vision_stack.launch.py ==="
echo ""

# Test 1: Check if all required packages are available
echo "Test 1: Check required packages"
for pkg in gst_receiver stereo_splitter detection_node tracker_node behavior_node visual_servo; do
  if ros2 pkg prefix $pkg > /dev/null 2>&1; then
    echo "  ✓ $pkg"
  else
    echo "  ✗ $pkg NOT FOUND"
    exit 1
  fi
done

echo ""
echo "Test 2: Verify launch file can be loaded"
timeout 2 ros2 launch robot_bringup vision_stack.launch.py 2>&1 | head -20 || true

echo ""
echo "Test 3: Check launch file Python syntax"
python3 -c "from launch import LaunchDescription; from robot_bringup.vision_stack import generate_launch_description; ld = generate_launch_description(); print(f'✓ Loaded {len(ld.launch_actions)} actions')" 2>/dev/null || echo "✗ Failed to load launch description"

echo ""
echo "All tests completed!"
