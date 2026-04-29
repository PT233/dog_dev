#pragma once

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "robot_interfaces/msg/simple_detection2_d_array.hpp"
#include <deque>
#include <memory>
#include <mutex>

namespace detection_node {

// 检测可视化节点
//
// 缓存最近一小段原始图像，收到 /tracked_objects 后按 header.stamp 找对应图像，
// 在图像上绘制检测框和 track_id，再发布 /camera/image_detected。
class DetectionVizNode : public rclcpp::Node {
public:
  DetectionVizNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Subscription<robot_interfaces::msg::SimpleDetection2DArray>::SharedPtr detection_sub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;
  std::mutex image_buffer_mutex_;
  std::deque<sensor_msgs::msg::Image::SharedPtr> image_buffer_;  // 按时间顺序保存最近图像

  // 缓存图像，等待检测结果到来后做时间戳匹配。
  void OnImage(const sensor_msgs::msg::Image::SharedPtr msg);

  // 检测结果回调：找到对应图像并发布叠加画面。
  void OnDetections(const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr msg);

  // 优先精确匹配时间戳，找不到时允许 250ms 内的最近图像兜底。
  sensor_msgs::msg::Image::SharedPtr FindImageForStamp(const builtin_interfaces::msg::Time& stamp);

  // 在图像上绘制检测框、类别、置信度和 track_id。
  void PublishOverlay(
      const sensor_msgs::msg::Image::SharedPtr& image_msg,
      const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr& detections_msg);
};

}  // namespace detection_node
