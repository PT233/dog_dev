#include "behavior_node/behavior_node.hpp"
#include "shared/ros2_single_node_main.hpp"

int main(int argc, char* argv[]) {
  return project_shared::spin_single_node_main<behavior_node::BehaviorNode>(argc, argv);
}
