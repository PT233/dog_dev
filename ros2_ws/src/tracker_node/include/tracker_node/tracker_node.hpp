#pragma once

#include "rclcpp/rclcpp.hpp"
#include "robot_interfaces/msg/simple_detection2_d_array.hpp"
#include "tracker_node/byte_tracker.hpp"
#include <vector>
#include <memory>

namespace tracker_node {

// 目标跟踪节点
// 输入：/detections，来自 YOLO 检测节点；
// 输出：/tracked_objects，保留原检测框并填入稳定 track_id。
class TrackerNode : public rclcpp::Node {
public:
  TrackerNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
  // ROS 2 订阅和发布句柄。
  rclcpp::Subscription<robot_interfaces::msg::SimpleDetection2DArray>::SharedPtr detections_sub_;
  rclcpp::Publisher<robot_interfaces::msg::SimpleDetection2DArray>::SharedPtr tracked_objects_pub_;

  // 简化版 ByteTracker 实例，保存跨帧轨迹状态。
  std::unique_ptr<ByteTracker> tracker_;

  // 检测回调：每帧做格式转换、跟踪、再转回自定义消息。
  void OnDetections(const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr msg);

  // 将 ROS 接口消息转换为 ByteTracker 使用的内部 Detection。
  Detection SimpleDetectionToByteTrack(const robot_interfaces::msg::SimpleDetection& det);
};

}  // namespace tracker_node
