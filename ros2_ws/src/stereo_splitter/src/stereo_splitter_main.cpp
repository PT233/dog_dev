#include "stereo_splitter/stereo_splitter_node.hpp"

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<StereoSplitterNode>());
  rclcpp::shutdown();
  return 0;
}
