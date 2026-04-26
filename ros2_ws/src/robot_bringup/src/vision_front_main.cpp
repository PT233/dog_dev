#include "rclcpp/rclcpp.hpp"
#include "gst_receiver/gst_receiver_node.hpp"
#include "stereo_splitter/stereo_splitter_node.hpp"
#include "detection_node/detection_node.hpp"

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);

  rclcpp::NodeOptions opts;
  opts.use_intra_process_comms(true);

  auto executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
  executor->add_node(std::make_shared<GstReceiverNode>(opts));
  executor->add_node(std::make_shared<StereoSplitterNode>(opts));
  executor->add_node(std::make_shared<DetectionNode>(opts));

  executor->spin();

  rclcpp::shutdown();
  return 0;
}
