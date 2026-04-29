#pragma once

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "robot_interfaces/msg/simple_detection2_d_array.hpp"
#include <deque>
#include <memory>
#include <mutex>

namespace detection_node {

class DetectionVizNode : public rclcpp::Node {
public:
  DetectionVizNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Subscription<robot_interfaces::msg::SimpleDetection2DArray>::SharedPtr detection_sub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;
  std::mutex image_buffer_mutex_;
  std::deque<sensor_msgs::msg::Image::SharedPtr> image_buffer_;

  void OnImage(const sensor_msgs::msg::Image::SharedPtr msg);
  void OnDetections(const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr msg);
  sensor_msgs::msg::Image::SharedPtr FindImageForStamp(const builtin_interfaces::msg::Time& stamp);
  void PublishOverlay(
      const sensor_msgs::msg::Image::SharedPtr& image_msg,
      const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr& detections_msg);
};

}  // namespace detection_node
