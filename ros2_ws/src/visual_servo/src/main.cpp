#include "shared/ros2_single_node_main.hpp"
#include "visual_servo/leg_motion_controller_node.hpp"

int main(int argc, char* argv[]) {
  return project_shared::spin_single_node_main<visual_servo::LegMotionControllerNode>(
      argc, argv);
}
