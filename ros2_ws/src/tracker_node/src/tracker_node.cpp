#include "tracker_node/tracker_node.hpp"

namespace tracker_node {

TrackerNode::TrackerNode(const rclcpp::NodeOptions& options)
    : rclcpp::Node("tracker_node", options) {
  // Get parameters from YAML
  int track_buffer = this->declare_parameter<int>("track_buffer", 30);
  float track_thresh = this->declare_parameter<double>("track_thresh", 0.5);
  float match_thresh = this->declare_parameter<double>("match_thresh", 0.5);

  // Initialize ByteTracker
  tracker_ = std::make_unique<ByteTracker>(track_buffer, track_thresh, match_thresh);

  // Create subscription to /detections
  auto qos = rclcpp::SensorDataQoS();
  detections_sub_ = this->create_subscription<robot_interfaces::msg::SimpleDetection2DArray>(
    "/detections", qos,
    std::bind(&TrackerNode::OnDetections, this, std::placeholders::_1));

  // Create publisher for /tracked_objects
  tracked_objects_pub_ =
    this->create_publisher<robot_interfaces::msg::SimpleDetection2DArray>(
      "/tracked_objects", rclcpp::QoS(5).reliable());

  RCLCPP_INFO(this->get_logger(), "TrackerNode initialized with track_buffer=%d, "
              "track_thresh=%.2f, match_thresh=%.2f",
              track_buffer, track_thresh, match_thresh);
}

void TrackerNode::OnDetections(const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr msg) {
  if (!msg || msg->detections.empty()) {
    // Publish empty result
    auto result = std::make_shared<robot_interfaces::msg::SimpleDetection2DArray>();
    result->header = msg->header;
    tracked_objects_pub_->publish(*result);
    return;
  }

  // Convert to ByteTrack format
  std::vector<Detection> detections;
  for (const auto& det : msg->detections) {
    detections.push_back(SimpleDetectionToByteTrack(det));
  }

  // Run tracker
  auto tracked = tracker_->Update(detections);

  // Build output message
  auto result = std::make_shared<robot_interfaces::msg::SimpleDetection2DArray>();
  result->header = msg->header;

  for (const auto& [track_id, detection] : tracked) {
    // Find corresponding input detection
    int best_idx = -1;
    float best_dist = 50.0f;

    for (size_t i = 0; i < msg->detections.size(); ++i) {
      float dx = msg->detections[i].center_x - detection.x;
      float dy = msg->detections[i].center_y - detection.y;
      float dist = std::sqrt(dx * dx + dy * dy);
      if (dist < best_dist) {
        best_dist = dist;
        best_idx = i;
      }
    }

    if (best_idx >= 0) {
      auto output_det = msg->detections[best_idx];
      output_det.track_id = std::to_string(track_id);
      result->detections.push_back(output_det);
    }
  }

  tracked_objects_pub_->publish(*result);
}

Detection TrackerNode::SimpleDetectionToByteTrack(
    const robot_interfaces::msg::SimpleDetection& det) {
  Detection result;
  result.x = det.center_x;
  result.y = det.center_y;
  result.w = det.width;
  result.h = det.height;
  result.conf = det.confidence;
  result.class_id = det.class_id;
  return result;
}

}  // namespace tracker_node
