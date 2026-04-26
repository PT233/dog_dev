#pragma once

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "robot_interfaces/msg/simple_detection2_d_array.hpp"
#include <memory>

namespace detection_node {

class DetectionVizNode : public rclcpp::Node {
public:
  DetectionVizNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Subscription<robot_interfaces::msg::SimpleDetection2DArray>::SharedPtr detection_sub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;

  void OnImage(const sensor_msgs::msg::Image::SharedPtr msg);
  void OnDetections(const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr msg);
};

}  // namespace detection_node
