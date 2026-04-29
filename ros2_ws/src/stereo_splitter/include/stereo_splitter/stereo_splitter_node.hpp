#ifndef STEREO_SPLITTER__STEREO_SPLITTER_NODE_HPP_
#define STEREO_SPLITTER__STEREO_SPLITTER_NODE_HPP_

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"

// 双目图像裁剪节点
//
// 输入是左右相机横向拼接图，本节点取左半幅 320x480，
// 发布到私有 left image 输出，供后续 YOLO 检测使用。
namespace stereo_splitter
{

class StereoSplitterNode : public rclcpp::Node {
public:
  explicit StereoSplitterNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr left_image_pub_;

  // 每帧拷贝左半幅图像，保持原始 header 时间戳。
  void stereo_image_callback(sensor_msgs::msg::Image::UniquePtr msg);
};

}  // namespace stereo_splitter

#endif  // STEREO_SPLITTER__STEREO_SPLITTER_NODE_HPP_
