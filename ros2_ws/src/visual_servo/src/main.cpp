#include "shared/ros2_single_node_main.hpp"
#include "visual_servo/leg_motion_controller_node.hpp"

int main(int argc, char * argv[])
{
  // 启动视觉伺服单节点：订阅私有输入并发布私有 servo_command 输出。
  return project_shared::spin_single_node_main<visual_servo::LegMotionControllerNode>(
      argc, argv);
}
