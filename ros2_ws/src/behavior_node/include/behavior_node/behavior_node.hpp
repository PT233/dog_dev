#pragma once

// 行为决策节点
// 从跟踪结果中按类别筛选目标，选择面积最大的候选，
// 计算其相对画面中心的像素偏差并发布，供腿部控制节点生成步态命令。
// 支持通过 /set_target_class 服务运行时切换跟踪目标类别。

#include "rclcpp/rclcpp.hpp"
#include "robot_interfaces/msg/simple_detection2_d_array.hpp"
#include "robot_interfaces/srv/set_target_class.hpp"
#include "geometry_msgs/msg/vector3.hpp"
#include <string>
#include <memory>
#include <map>

namespace behavior_node {

// 行为决策节点
// 订阅：/tracked_objects（来自 tracker_node）
// 发布：/pixel_error (Vector3) = (target_cx - center_x, target_cy - center_y, 0)
// 服务：/set_target_class，动态切换跟踪的 COCO 类别
class BehaviorNode : public rclcpp::Node {
public:
  BehaviorNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
  rclcpp::Subscription<robot_interfaces::msg::SimpleDetection2DArray>::SharedPtr tracked_objects_sub_;
  rclcpp::Publisher<geometry_msgs::msg::Vector3>::SharedPtr pixel_error_pub_;
  rclcpp::Service<robot_interfaces::srv::SetTargetClass>::SharedPtr set_target_class_srv_;

  // 当前跟踪目标类别（COCO class_id，默认 0 = person）
  int target_class_id_ = 0;
  int current_track_id_ = -1;  // 当前锁定的 track_id（-1 表示未锁定）
  int target_cx_ = 0;          // 目标中心像素 X（从最近一帧更新）
  int target_cy_ = 0;          // 目标中心像素 Y

  // 图像参数（从 YAML 加载）
  int image_width_ = 320;
  int image_height_ = 480;
  int center_x_ = 160;   // 画面中心 X（可通过 /calibrate_center 调整）
  int center_y_ = 240;   // 画面中心 Y

  // COCO 类别 ID → 名称映射（从 coco_classes.txt 加载）
  std::map<int, std::string> coco_classes_;

  // Callbacks
  void OnTrackedObjects(const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr msg);
  void OnSetTargetClass(
      const std::shared_ptr<robot_interfaces::srv::SetTargetClass::Request> request,
      std::shared_ptr<robot_interfaces::srv::SetTargetClass::Response> response);

  // Helper functions
  bool SelectTarget(const robot_interfaces::msg::SimpleDetection2DArray& detections);
  void LoadCocoClasses();
  int GetClassIdByName(const std::string& class_name) const;
};

}  // namespace behavior_node
