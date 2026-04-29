#include "tracker_node/tracker_node.hpp"

namespace tracker_node {

TrackerNode::TrackerNode(const rclcpp::NodeOptions& options)
    : rclcpp::Node("tracker_node", options) {
  // 跟踪参数来自 YAML：阈值越高越保守，track_buffer 越大越能容忍短暂遮挡。
  int track_buffer = this->declare_parameter<int>("track_buffer", 30);
  float track_thresh = this->declare_parameter<double>("track_thresh", 0.5);
  float match_thresh = this->declare_parameter<double>("match_thresh", 0.5);

  // 创建跟踪器实例，内部维护 active_tracks_ 和自增 track_id。
  tracker_ = std::make_unique<ByteTracker>(track_buffer, track_thresh, match_thresh);

  // 订阅检测结果，SensorDataQoS 优先低延迟，允许丢弃过期视觉帧。
  auto qos = rclcpp::SensorDataQoS();
  detections_sub_ = this->create_subscription<robot_interfaces::msg::SimpleDetection2DArray>(
    "/detections", qos,
    std::bind(&TrackerNode::OnDetections, this, std::placeholders::_1));

  // 跟踪结果给 behavior_node 使用，使用 reliable 降低本机进程间丢消息概率。
  tracked_objects_pub_ =
    this->create_publisher<robot_interfaces::msg::SimpleDetection2DArray>(
      "/tracked_objects", rclcpp::QoS(5).reliable());

  RCLCPP_INFO(this->get_logger(), "TrackerNode initialized with track_buffer=%d, "
              "track_thresh=%.2f, match_thresh=%.2f",
              track_buffer, track_thresh, match_thresh);
}

void TrackerNode::OnDetections(const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr msg) {
  if (!msg || msg->detections.empty()) {
    // 发布空数组而不是静默返回，让下游知道当前帧确实没有目标。
    auto result = std::make_shared<robot_interfaces::msg::SimpleDetection2DArray>();
    result->header = msg->header;
    tracked_objects_pub_->publish(*result);
    return;
  }

  // 自定义 ROS 消息转为 ByteTracker 内部中心点格式。
  std::vector<Detection> detections;
  for (const auto& det : msg->detections) {
    detections.push_back(SimpleDetectionToByteTrack(det));
  }

  // 跟踪器返回 track_id 与检测框的关联结果。
  auto tracked = tracker_->Update(detections);

  // 输出消息沿用原始 header，便于可视化节点按时间戳找对应图像。
  auto result = std::make_shared<robot_interfaces::msg::SimpleDetection2DArray>();
  result->header = msg->header;

  for (const auto& [track_id, detection] : tracked) {
    // 根据中心点距离找回原始检测消息，保留 confidence/class_id 等字段。
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
  // SimpleDetection 已经使用中心点宽高格式，字段可直接复制。
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
