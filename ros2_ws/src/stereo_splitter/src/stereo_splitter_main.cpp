#include "stereo_splitter/stereo_splitter_node.hpp"
#include "shared/ros2_single_node_main.hpp"

int main(int argc, char *argv[])
{
  // 启动图像裁剪节点，把双目拼接图的左半幅发布到私有输出。
  return project_shared::spin_single_node_main<stereo_splitter::StereoSplitterNode>(argc, argv);
}
