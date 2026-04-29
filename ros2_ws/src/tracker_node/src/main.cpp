#include "tracker_node/tracker_node.hpp"
#include "shared/ros2_single_node_main.hpp"

int main(int argc, char * argv[])
{
  // 启动目标跟踪单节点，将私有检测输入关联为带 track_id 的私有跟踪输出。
  return project_shared::spin_single_node_main<tracker_node::TrackerNode>(argc, argv);
}
