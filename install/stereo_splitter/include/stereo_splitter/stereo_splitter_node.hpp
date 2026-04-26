#pragma once

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"

class StereoSplitterNode : public rclcpp::Node {
public:
  explicit StereoSplitterNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr left_pub_;

  void image_callback(sensor_msgs::msg::Image::UniquePtr msg);
};
