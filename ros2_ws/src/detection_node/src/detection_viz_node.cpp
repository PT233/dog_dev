#include "detection_node/detection_viz_node.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

#include <opencv2/imgproc.hpp>

namespace detection_node {

namespace {

constexpr size_t kImageBufferSize = 15;
constexpr uint64_t kFallbackToleranceNs = 250000000ULL;  // 250 ms

uint64_t StampToNs(const builtin_interfaces::msg::Time& stamp) {
  // ROS 时间戳转成纳秒整数，便于比较和求差。
  return static_cast<uint64_t>(stamp.sec) * 1000000000ULL + stamp.nanosec;
}

cv::Scalar ColorForDetection(const robot_interfaces::msg::SimpleDetection& det) {
  // 有 track_id 时按轨迹着色，否则按 class_id 着色，便于观察 ID 是否稳定。
  const std::string key = det.track_id.empty()
                              ? std::to_string(det.class_id)
                              : det.track_id;
  size_t hash = std::hash<std::string>{}(key);
  const int b = 80 + static_cast<int>(hash & 0x7F);
  const int g = 80 + static_cast<int>((hash >> 8) & 0x7F);
  const int r = 80 + static_cast<int>((hash >> 16) & 0x7F);
  return cv::Scalar(b, g, r);
}

std::string BuildLabel(const robot_interfaces::msg::SimpleDetection& det) {
  // 标签格式尽量短，避免遮挡图像主体：ID、类别、置信度。
  std::ostringstream oss;
  if (!det.track_id.empty()) {
    oss << "ID:" << det.track_id << " ";
  }
  oss << "C:" << det.class_id << " ";
  oss.setf(std::ios::fixed);
  oss.precision(2);
  oss << det.confidence;
  return oss.str();
}

}  // namespace

DetectionVizNode::DetectionVizNode(const rclcpp::NodeOptions& options)
    : rclcpp::Node("detection_viz_node", options) {
  auto qos = rclcpp::SensorDataQoS();

  // 原始图像用于叠加绘制，跟踪结果用于确定框位置和 ID。
  image_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
    "/camera/image_mono", qos,
    std::bind(&DetectionVizNode::OnImage, this, std::placeholders::_1));

  detection_sub_ = this->create_subscription<robot_interfaces::msg::SimpleDetection2DArray>(
    "/tracked_objects", rclcpp::QoS(5).reliable(),
    std::bind(&DetectionVizNode::OnDetections, this, std::placeholders::_1));

  // 发布叠加后的图像，供 rqt_image_view 或录包检查视觉链路。
  image_pub_ = this->create_publisher<sensor_msgs::msg::Image>(
    "/camera/image_detected", qos);

  RCLCPP_INFO(this->get_logger(), "DetectionVizNode started");
  RCLCPP_INFO(this->get_logger(), "Subscribed to /camera/image_mono and /tracked_objects");
  RCLCPP_INFO(this->get_logger(), "Publishing tracked overlay to /camera/image_detected");
}

void DetectionVizNode::OnImage(const sensor_msgs::msg::Image::SharedPtr msg) {
  if (!msg) {
    return;
  }

  std::lock_guard<std::mutex> lock(image_buffer_mutex_);
  image_buffer_.push_back(msg);
  // 固定缓存长度，防止可视化节点长时间运行时积累图像内存。
  while (image_buffer_.size() > kImageBufferSize) {
    image_buffer_.pop_front();
  }
}

void DetectionVizNode::OnDetections(
    const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr msg) {
  if (!msg) {
    return;
  }

  auto image_msg = FindImageForStamp(msg->header.stamp);
  if (!image_msg) {
    RCLCPP_WARN_THROTTLE(
        this->get_logger(), *this->get_clock(), 2000,
        "No buffered image available for tracked_objects stamp; overlay frame skipped");
    return;
  }

  PublishOverlay(image_msg, msg);
}

sensor_msgs::msg::Image::SharedPtr DetectionVizNode::FindImageForStamp(
    const builtin_interfaces::msg::Time& stamp) {
  std::lock_guard<std::mutex> lock(image_buffer_mutex_);
  if (image_buffer_.empty()) {
    return nullptr;
  }

  const uint64_t target_ns = StampToNs(stamp);
  for (auto it = image_buffer_.rbegin(); it != image_buffer_.rend(); ++it) {
    // 先从最新图像往回找精确时间戳匹配。
    if (StampToNs((*it)->header.stamp) == target_ns) {
      return *it;
    }
  }

  sensor_msgs::msg::Image::SharedPtr fallback = nullptr;
  uint64_t best_diff = UINT64_MAX;
  // 检测和图像时间戳偶尔不完全一致时，允许一个小容差内最近帧兜底。
  for (auto it = image_buffer_.rbegin(); it != image_buffer_.rend(); ++it) {
    const uint64_t image_ns = StampToNs((*it)->header.stamp);
    const uint64_t diff = (image_ns > target_ns) ? (image_ns - target_ns) : (target_ns - image_ns);
    if (diff < best_diff) {
      best_diff = diff;
      fallback = *it;
    }
  }

  if (best_diff <= kFallbackToleranceNs) {
    return fallback;
  }
  return nullptr;
}

void DetectionVizNode::PublishOverlay(
    const sensor_msgs::msg::Image::SharedPtr& image_msg,
    const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr& detections_msg) {
  auto overlay_msg = std::make_unique<sensor_msgs::msg::Image>(*image_msg);
  if (overlay_msg->encoding != "bgr8") {
    // 当前绘制逻辑按 BGR 三通道写像素，非 bgr8 时直接透传，避免错误解释内存。
    RCLCPP_WARN_THROTTLE(
        this->get_logger(), *this->get_clock(), 2000,
        "Expected bgr8 image for overlay, got '%s'; forwarding raw image",
        overlay_msg->encoding.c_str());
    image_pub_->publish(std::move(overlay_msg));
    return;
  }

  cv::Mat image(
      static_cast<int>(overlay_msg->height),
      static_cast<int>(overlay_msg->width),
      CV_8UC3,
      overlay_msg->data.data(),
      overlay_msg->step);

  for (const auto& det : detections_msg->detections) {
    // 中心点宽高格式转为左上/右下角，并限制在图像边界内。
    const int x1 = std::max(0, static_cast<int>(std::lround(det.center_x - det.width * 0.5f)));
    const int y1 = std::max(0, static_cast<int>(std::lround(det.center_y - det.height * 0.5f)));
    const int x2 = std::min(
        static_cast<int>(overlay_msg->width) - 1,
        static_cast<int>(std::lround(det.center_x + det.width * 0.5f)));
    const int y2 = std::min(
        static_cast<int>(overlay_msg->height) - 1,
        static_cast<int>(std::lround(det.center_y + det.height * 0.5f)));

    if (x2 <= x1 || y2 <= y1) {
      continue;
    }

    const cv::Scalar color = ColorForDetection(det);
    cv::rectangle(image, cv::Point(x1, y1), cv::Point(x2, y2), color, 2);

    const std::string label = BuildLabel(det);
    int baseline = 0;
    const cv::Size text_size =
        cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.45, 1, &baseline);

    const int label_x = x1;
    const int label_y = std::max(text_size.height + 6, y1 - 6);
    const cv::Rect bg_rect(
        label_x,
        label_y - text_size.height - 4,
        text_size.width + 8,
        text_size.height + 6);

    cv::rectangle(image, bg_rect, color, cv::FILLED);
    cv::putText(
        image,
        label,
        cv::Point(label_x + 4, label_y - 4),
        cv::FONT_HERSHEY_SIMPLEX,
        0.45,
        cv::Scalar(255, 255, 255),
        1,
        cv::LINE_AA);
  }

  overlay_msg->header = detections_msg->header;
  image_pub_->publish(std::move(overlay_msg));
}

}  // namespace detection_node
