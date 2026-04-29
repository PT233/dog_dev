#include "shared/ros2_single_node_main.hpp"
#include "detection_node/detection_node.hpp"

int main(int argc, char* argv[]) {
  // 启动 YOLO 检测节点；NodeOptions 保留显式传入，便于后续开启进程内通信。
  return project_shared::spin_single_node_main<DetectionNode>(
      argc, argv, rclcpp::NodeOptions{});
}
