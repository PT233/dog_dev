#include "rclcpp/rclcpp.hpp"
#include "detection_node/detection_node.hpp"
#include "gst_receiver/gst_receiver_node.hpp"
#include "stereo_splitter/stereo_splitter_node.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  // 三个视觉前端节点放在同一进程内，减少 ROS 消息跨进程拷贝。
  rclcpp::NodeOptions gst_receiver_options;
  gst_receiver_options.use_intra_process_comms(true);

  rclcpp::NodeOptions stereo_splitter_options;
  stereo_splitter_options.use_intra_process_comms(true);
  stereo_splitter_options.arguments({
    "--ros-args", "-r",
    "~/input/stereo_image_raw:=gst_receiver_node/output/stereo_image_raw"});

  rclcpp::NodeOptions detection_options;
  detection_options.use_intra_process_comms(true);
  detection_options.arguments({
    "--ros-args", "-r", "~/input/image:=stereo_splitter_node/output/left_image"});

  // MultiThreadedExecutor 允许 GStreamer 回调、裁剪和检测节点并行推进。
  auto executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
  executor->add_node(std::make_shared<gst_receiver::GstReceiverNode>(gst_receiver_options));
  executor->add_node(std::make_shared<stereo_splitter::StereoSplitterNode>(
      stereo_splitter_options));
  executor->add_node(std::make_shared<detection_node::DetectionNode>(detection_options));

  executor->spin();

  rclcpp::shutdown();
  return 0;
}
