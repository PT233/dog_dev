#include "tracker_node/tracker_node.hpp"

namespace tracker_node
{

TrackerNode::TrackerNode(const rclcpp::NodeOptions & options)
: rclcpp::Node("tracker_node", options)
{
  // 跟踪参数来自 YAML：阈值越高越保守，track_buffer 越大越能容忍短暂遮挡。
  int track_buffer = this->declare_parameter<int>("tracking.track_buffer", 30);
  float confidence_threshold =
    this->declare_parameter<double>("tracking.confidence_threshold", 0.5);
  float match_threshold = this->declare_parameter<double>("tracking.match_threshold", 0.5);

  // 创建跟踪器实例，内部维护 active_tracks_ 和自增 track_id。
  tracker_ = std::make_unique<ByteTracker>(track_buffer, confidence_threshold, match_threshold);

  // 订阅检测结果，SensorDataQoS 优先低延迟，允许丢弃过期视觉帧。
  auto qos = rclcpp::SensorDataQoS();
  detections_sub_ = this->create_subscription<robot_interfaces::msg::Detection2DArray>(
    "~/input/detections", qos,
    std::bind(&TrackerNode::detections_callback, this, std::placeholders::_1));

  // 跟踪结果给 behavior_node 使用，使用 reliable 降低本机进程间丢消息概率。
  tracked_objects_pub_ =
    this->create_publisher<robot_interfaces::msg::Detection2DArray>(
      "~/output/tracked_objects", rclcpp::QoS(5).reliable());

  RCLCPP_INFO(this->get_logger(), "TrackerNode initialized with track_buffer=%d, "
              "confidence_threshold=%.2f, match_threshold=%.2f",
              track_buffer, confidence_threshold, match_threshold);
}

void TrackerNode::detections_callback(
  const robot_interfaces::msg::Detection2DArray::SharedPtr msg)
{
  if (!msg || msg->detections.empty()) {
    // 发布空数组而不是静默返回，让下游知道当前帧确实没有目标。
    auto result = std::make_shared<robot_interfaces::msg::Detection2DArray>();
    if (msg) {
      result->header = msg->header;
    }
    tracked_objects_pub_->publish(*result);
    return;
  }

  // 自定义 ROS 消息转为 ByteTracker 内部中心点格式。
  std::vector<Detection> detections;
  for (const auto & detection_msg : msg->detections) {
    detections.push_back(detection_from_msg(detection_msg));
  }

  // 跟踪器返回 track_id 与检测框的关联结果。
  auto tracked = tracker_->update(detections);

  // 输出消息沿用原始 header，便于可视化节点按时间戳找对应图像。
  auto result = std::make_shared<robot_interfaces::msg::Detection2DArray>();
  result->header = msg->header;

  for (const auto & [track_id, detection] : tracked) {
    // 根据中心点距离找回原始检测消息，保留 confidence/class_id 等字段。
    int best_detection_index = -1;
    float best_distance = 50.0f;

    for (size_t i = 0; i < msg->detections.size(); ++i) {
      float delta_x = msg->detections[i].center_x - detection.center_x;
      float delta_y = msg->detections[i].center_y - detection.center_y;
      float distance = std::sqrt(delta_x * delta_x + delta_y * delta_y);
      if (distance < best_distance) {
        best_distance = distance;
        best_detection_index = static_cast<int>(i);
      }
    }

    if (best_detection_index >= 0) {
      auto output_detection = msg->detections[best_detection_index];
      output_detection.track_id = std::to_string(track_id);
      result->detections.push_back(output_detection);
    }
  }

  tracked_objects_pub_->publish(*result);
}

Detection TrackerNode::detection_from_msg(
  const robot_interfaces::msg::Detection2D & detection_msg)
{
  // Detection2D 已经使用中心点宽高格式，字段可直接复制。
  Detection result;
  result.center_x = detection_msg.center_x;
  result.center_y = detection_msg.center_y;
  result.width = detection_msg.width;
  result.height = detection_msg.height;
  result.confidence = detection_msg.confidence;
  result.class_id = detection_msg.class_id;
  return result;
}

}  // namespace tracker_node
