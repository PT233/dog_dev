#include "shared/ros2_single_node_main.hpp"
#include "visual_servo/leg_motion_controller_node.hpp"

int main(int argc, char* argv[]) {
  // 启动视觉伺服单节点：订阅 /pixel_error 和 /servo_state，发布 /servo_cmd。
  return project_shared::spin_single_node_main<visual_servo::LegMotionControllerNode>(
      argc, argv);
}
