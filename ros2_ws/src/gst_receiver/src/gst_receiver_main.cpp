#include "gst_receiver/gst_receiver_node.hpp"

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<GstReceiverNode>());
  rclcpp::shutdown();
  return 0;
}
