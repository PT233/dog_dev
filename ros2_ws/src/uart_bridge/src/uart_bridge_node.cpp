#include <rclcpp/rclcpp.hpp>

class UartBridgeNode : public rclcpp::Node {
public:
  UartBridgeNode() : Node("uart_bridge_node") {
    RCLCPP_INFO(this->get_logger(), "uart_bridge_node started");
  }
};

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UartBridgeNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
