#include "stereo_splitter/stereo_splitter_node.hpp"
#include "shared/ros2_single_node_main.hpp"

int main(int argc, char *argv[]) {
  return project_shared::spin_single_node_main<StereoSplitterNode>(argc, argv);
}
