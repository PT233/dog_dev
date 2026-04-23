#include "stereo_splitter/stereo_splitter_node.hpp"
#include <opencv2/opencv.hpp>

StereoSplitterNode::StereoSplitterNode() : Node("stereo_splitter_node") {
  RCLCPP_INFO(this->get_logger(), "stereo_splitter_node started");

  image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
    "/stereo/image_raw",
    rclcpp::SensorDataQoS(),
    std::bind(&StereoSplitterNode::image_callback, this, std::placeholders::_1));

  left_pub_ = this->create_publisher<sensor_msgs::msg::Image>(
    "/camera/image_mono",
    rclcpp::SensorDataQoS());
}

void StereoSplitterNode::image_callback(const sensor_msgs::msg::Image::SharedPtr msg) {
  // Convert ROS Image message to cv::Mat
  int width = msg->width;
  int height = msg->height;

  cv::Mat full_image(height, width, CV_8UC3, (void *)msg->data.data());

  // Extract left half: [0, 320) x [0, 480)
  cv::Rect roi(0, 0, 320, 480);
  cv::Mat left_image = full_image(roi).clone();

  // Convert back to ROS Image message
  sensor_msgs::msg::Image left_msg;
  left_msg.header = msg->header;
  left_msg.height = left_image.rows;
  left_msg.width = left_image.cols;
  left_msg.encoding = "bgr8";
  left_msg.is_bigendian = false;
  left_msg.step = left_image.cols * 3;
  left_msg.data.assign(left_image.data, left_image.data + (left_image.rows * left_image.cols * 3));

  left_pub_->publish(left_msg);

  RCLCPP_DEBUG(this->get_logger(), "Published left image: 320x480");
}

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<StereoSplitterNode>());
  rclcpp::shutdown();
  return 0;
}
