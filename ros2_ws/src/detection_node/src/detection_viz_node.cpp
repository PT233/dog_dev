#include "detection_node/detection_viz_node.hpp"

namespace detection_node {

DetectionVizNode::DetectionVizNode(const rclcpp::NodeOptions& options)
    : rclcpp::Node("detection_viz_node", options) {
  auto qos = rclcpp::SensorDataQoS();

  // Create subscriptions
  image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
    "/camera/image_mono", qos,
    std::bind(&DetectionVizNode::OnImage, this, std::placeholders::_1));

  detection_sub_ = this->create_subscription<robot_interfaces::msg::SimpleDetection2DArray>(
    "/tracked_objects", rclcpp::QoS(5).reliable(),
    std::bind(&DetectionVizNode::OnDetections, this, std::placeholders::_1));

  // Create publisher
  image_pub_ = this->create_publisher<sensor_msgs::msg::Image>(
    "/camera/image_detected", qos);

  RCLCPP_INFO(this->get_logger(), "DetectionVizNode started");
  RCLCPP_INFO(this->get_logger(), "Subscribed to /camera/image_mono and /tracked_objects");
  RCLCPP_INFO(this->get_logger(), "Publishing to /camera/image_detected");
}

void DetectionVizNode::OnImage(const sensor_msgs::msg::Image::SharedPtr msg) {
  // For now, just republish
  image_pub_->publish(*msg);
}

void DetectionVizNode::OnDetections(
    const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr msg) {
  // Log track IDs received
  std::string track_ids;
  for (const auto& det : msg->detections) {
    track_ids += "ID:" + det.track_id + " ";
  }
  if (!track_ids.empty()) {
    RCLCPP_DEBUG(this->get_logger(), "Tracked objects: %s", track_ids.c_str());
  }
}

}  // namespace detection_node
