#include "rclcpp/rclcpp.hpp"
#include "tracker_node/tracker_node.hpp"

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<tracker_node::TrackerNode>());
  rclcpp::shutdown();
  return 0;
}
