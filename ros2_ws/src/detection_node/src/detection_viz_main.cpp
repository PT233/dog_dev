#include "detection_node/detection_viz_node.hpp"
#include "shared/ros2_single_node_main.hpp"

int main(int argc, char * argv[])
{
  // 启动检测可视化节点，在图像上叠加 tracked_objects 检测框。
  return project_shared::spin_single_node_main<detection_node::DetectionVizNode>(argc, argv);
}
