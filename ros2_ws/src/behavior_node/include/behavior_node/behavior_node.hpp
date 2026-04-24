#pragma once

#include "rclcpp/rclcpp.hpp"
#include "robot_interfaces/msg/simple_detection2_d_array.hpp"
#include "robot_interfaces/srv/set_target_class.hpp"
#include "geometry_msgs/msg/vector3.hpp"
#include <string>
#include <memory>
#include <map>

namespace behavior_node {

class BehaviorNode : public rclcpp::Node {
public:
  BehaviorNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

private:
  // Subscriptions and publishers
  rclcpp::Subscription<robot_interfaces::msg::SimpleDetection2DArray>::SharedPtr tracked_objects_sub_;
  rclcpp::Publisher<geometry_msgs::msg::Vector3>::SharedPtr pixel_error_pub_;

  // Service
  rclcpp::Service<robot_interfaces::srv::SetTargetClass>::SharedPtr set_target_class_srv_;

  // Target tracking state
  int target_class_id_ = 0;  // Default: person (class_id 0 in COCO)
  int current_track_id_ = -1;
  int target_cx_ = 0;
  int target_cy_ = 0;

  // Image parameters
  int image_width_ = 320;
  int image_height_ = 480;
  int center_x_ = 160;
  int center_y_ = 240;

  // Class ID to name mapping (COCO dataset)
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
