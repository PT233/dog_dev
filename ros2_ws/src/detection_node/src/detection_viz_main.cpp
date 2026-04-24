#include "rclcpp/rclcpp.hpp"
#include "detection_node/detection_viz_node.hpp"

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<detection_node::DetectionVizNode>());
  rclcpp::shutdown();
  return 0;
}
