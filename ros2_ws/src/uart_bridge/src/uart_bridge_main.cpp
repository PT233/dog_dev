#include "shared/ros2_single_node_main.hpp"
#include "uart_bridge_node_internal.hpp"

int main(int argc, char* argv[]) {
  // 使用项目通用单节点入口，负责 rclcpp 初始化、spin 和 shutdown。
  return project_shared::spin_single_node_main<UartBridgeNode>(argc, argv);
}
