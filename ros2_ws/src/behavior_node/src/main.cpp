#include "behavior_node/behavior_node.hpp"
#include "shared/ros2_single_node_main.hpp"

int main(int argc, char* argv[]) {
  // 通用入口负责 rclcpp 初始化、创建 BehaviorNode、spin 以及 shutdown。
  return project_shared::spin_single_node_main<behavior_node::BehaviorNode>(argc, argv);
}
