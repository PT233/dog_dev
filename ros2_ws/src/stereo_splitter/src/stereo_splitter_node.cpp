#include "stereo_splitter/stereo_splitter_node.hpp"
#include <opencv2/opencv.hpp>

StereoSplitterNode::StereoSplitterNode(const rclcpp::NodeOptions& options)
    : Node("stereo_splitter_node", options) {
  RCLCPP_INFO(this->get_logger(), "stereo_splitter_node started");

  // 输入为 gst_receiver 发布的左右拼接 BGR 图。
  image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
    "/stereo/image_raw",
    rclcpp::SensorDataQoS(),
    std::bind(&StereoSplitterNode::image_callback, this, std::placeholders::_1));

  // 下游节点当前只使用左目图像做单目检测。
  left_pub_ = this->create_publisher<sensor_msgs::msg::Image>(
    "/camera/image_mono",
    rclcpp::SensorDataQoS());
}

void StereoSplitterNode::image_callback(sensor_msgs::msg::Image::UniquePtr msg) {
  if (!msg) return;

  int width = msg->width;
  int height = msg->height;

  // 创建左目图像消息；逐行 memcpy，避免假设源图 step 正好等于 width*3。
  auto left_msg = std::make_unique<sensor_msgs::msg::Image>();
  left_msg->header = msg->header;
  left_msg->height = 480;
  left_msg->width = 320;
  left_msg->encoding = "bgr8";
  left_msg->is_bigendian = false;
  left_msg->step = 320 * 3;
  left_msg->data.resize(480 * 320 * 3);

  // 提取左半幅：[0, 320) x [0, 480)。当前相机固定输出 640x480 拼接图。
  // 逐行复制可以正确处理 GStreamer 输出中可能存在的行对齐 padding。
  const uint8_t* src = msg->data.data();
  uint8_t* dst = left_msg->data.data();
  for (int r = 0; r < 480; ++r) {
    std::memcpy(dst + r * (320 * 3), src + r * msg->step, 320 * 3);
  }

  left_pub_->publish(std::move(left_msg));

  RCLCPP_DEBUG(this->get_logger(), "Published left image: 320x480");
}
