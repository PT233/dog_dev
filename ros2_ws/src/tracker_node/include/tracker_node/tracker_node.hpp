#pragma once

#include "rclcpp/rclcpp.hpp"
#include "robot_interfaces/msg/simple_detection2_d_array.hpp"
#include "tracker_node/byte_tracker.hpp"
#include <vector>
#include <memory>

namespace tracker_node {

class TrackerNode : public rclcpp::Node {
public:
  TrackerNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
  // ROS 2 subscriptions and publishers
  rclcpp::Subscription<robot_interfaces::msg::SimpleDetection2DArray>::SharedPtr detections_sub_;
  rclcpp::Publisher<robot_interfaces::msg::SimpleDetection2DArray>::SharedPtr tracked_objects_pub_;

  // ByteTracker instance
  std::unique_ptr<ByteTracker> tracker_;

  // Callback for detections
  void OnDetections(const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr msg);

  // Convert to ByteTrack format
  Detection SimpleDetectionToByteTrack(const robot_interfaces::msg::SimpleDetection& det);
};

}  // namespace tracker_node
