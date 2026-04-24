#include "rclcpp/rclcpp.hpp"
#include "behavior_node/behavior_node.hpp"

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<behavior_node::BehaviorNode>());
  rclcpp::shutdown();
  return 0;
}
