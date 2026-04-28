#include "shared/ros2_single_node_main.hpp"
#include "uart_bridge_node_internal.hpp"

int main(int argc, char* argv[]) {
  return project_shared::spin_single_node_main<UartBridgeNode>(argc, argv);
}
