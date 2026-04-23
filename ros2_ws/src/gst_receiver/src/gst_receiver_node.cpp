#include "rclcpp/rclcpp.hpp"

class GstReceiverNode : public rclcpp::Node {
public:
  GstReceiverNode() : Node("gst_receiver_node") {
    RCLCPP_INFO(this->get_logger(), "gst_receiver_node started");
  }
};

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<GstReceiverNode>());
  rclcpp::shutdown();
  return 0;
}
