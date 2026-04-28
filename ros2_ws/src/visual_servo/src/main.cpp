#include "rclcpp/rclcpp.hpp"
#include "visual_servo/visual_servo_node.hpp"

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<visual_servo::LegMotionControllerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
