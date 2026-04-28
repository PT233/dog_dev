#include "shared/ros2_single_node_main.hpp"
#include "detection_node/detection_node.hpp"

int main(int argc, char* argv[]) {
  return project_shared::spin_single_node_main<DetectionNode>(
      argc, argv, rclcpp::NodeOptions{});
}
