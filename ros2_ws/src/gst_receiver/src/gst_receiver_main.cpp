#include "gst_receiver/gst_receiver_node.hpp"
#include "shared/ros2_single_node_main.hpp"

int main(int argc, char *argv[])
{
  // 启动 GStreamer 接收节点，接收 UDP/H.264 并发布私有图像输出。
  return project_shared::spin_single_node_main<gst_receiver::GstReceiverNode>(argc, argv);
}
