#include "behavior_node/behavior_node.hpp"

#include "shared/load_trimmed_lines.hpp"

namespace behavior_node {

BehaviorNode::BehaviorNode(const rclcpp::NodeOptions& options)
    : rclcpp::Node("behavior_node", options) {
  // Load parameters
  this->declare_parameter<int>("image_width", 320);
  this->declare_parameter<int>("image_height", 480);
  this->declare_parameter<int>("center_x", 160);
  this->declare_parameter<int>("center_y", 240);

  image_width_ = this->get_parameter("image_width").as_int();
  image_height_ = this->get_parameter("image_height").as_int();
  center_x_ = this->get_parameter("center_x").as_int();
  center_y_ = this->get_parameter("center_y").as_int();

  // Load COCO class names
  LoadCocoClasses();

  // Create subscription to tracked objects
  auto qos = rclcpp::SensorDataQoS();
  tracked_objects_sub_ =
    this->create_subscription<robot_interfaces::msg::SimpleDetection2DArray>(
      "/tracked_objects", qos,
      std::bind(&BehaviorNode::OnTrackedObjects, this, std::placeholders::_1));

  // Create publisher for pixel error
  pixel_error_pub_ =
    this->create_publisher<geometry_msgs::msg::Vector3>("/pixel_error", rclcpp::QoS(5));

  // Create service for setting target class
  set_target_class_srv_ =
    this->create_service<robot_interfaces::srv::SetTargetClass>(
      "/set_target_class",
      std::bind(&BehaviorNode::OnSetTargetClass, this, std::placeholders::_1, std::placeholders::_2));

  std::string class_name = "person";
  if (coco_classes_.count(target_class_id_)) {
    class_name = coco_classes_[target_class_id_];
  }
  RCLCPP_INFO(this->get_logger(), "BehaviorNode initialized (target: %s, class_id=%d, center: %d,%d)",
              class_name.c_str(), target_class_id_, center_x_, center_y_);
}

void BehaviorNode::OnTrackedObjects(
    const robot_interfaces::msg::SimpleDetection2DArray::SharedPtr msg) {
  if (!msg || msg->detections.empty()) {
    RCLCPP_INFO(this->get_logger(), "No target");
    return;
  }

  // Select target from detections
  if (SelectTarget(*msg)) {
    std::string class_name = "unknown";
    if (coco_classes_.count(target_class_id_)) {
      class_name = coco_classes_[target_class_id_];
    }
    RCLCPP_INFO(this->get_logger(),
                "Target: id=%d, center=(%d, %d), class=%s",
                current_track_id_, target_cx_, target_cy_, class_name.c_str());

    // Compute and publish pixel error
    auto error = geometry_msgs::msg::Vector3();
    error.x = target_cx_ - center_x_;
    error.y = target_cy_ - center_y_;
    error.z = 0.0;
    pixel_error_pub_->publish(error);
  } else {
    RCLCPP_INFO(this->get_logger(), "No target");
  }
}

bool BehaviorNode::SelectTarget(
    const robot_interfaces::msg::SimpleDetection2DArray& detections) {
  int best_idx = -1;
  float max_area = 0.0f;

  // 策略：从同类别目标中选面积最大的（即最近/最显著的目标）
  // 面积大通常意味着目标距摄像头更近，是最优跟踪候选
  for (size_t i = 0; i < detections.detections.size(); ++i) {
    const auto& det = detections.detections[i];

    if (det.class_id != target_class_id_) {
      continue;
    }

    float area = det.width * det.height;
    if (area > max_area) {
      max_area = area;
      best_idx = i;
    }
  }

  if (best_idx < 0) {
    return false;
  }

  const auto& target = detections.detections[best_idx];
  target_cx_ = static_cast<int>(target.center_x);
  target_cy_ = static_cast<int>(target.center_y);
  current_track_id_ = std::stoi(target.track_id);

  return true;
}

void BehaviorNode::LoadCocoClasses() {
  const std::string coco_classes_file = "models/coco_classes.txt";
  std::vector<std::string> class_names;

  if (!project_shared::load_trimmed_lines(coco_classes_file, &class_names)) {
    RCLCPP_WARN(this->get_logger(), "Could not open COCO classes file: %s",
                coco_classes_file.c_str());
    return;
  }

  for (size_t i = 0; i < class_names.size(); ++i) {
    coco_classes_[static_cast<int>(i)] = class_names[i];
  }

  RCLCPP_DEBUG(this->get_logger(), "Loaded %zu COCO classes", coco_classes_.size());
}

void BehaviorNode::OnSetTargetClass(
    const std::shared_ptr<robot_interfaces::srv::SetTargetClass::Request> request,
    std::shared_ptr<robot_interfaces::srv::SetTargetClass::Response> response) {
  int class_id = GetClassIdByName(request->class_name);

  if (class_id < 0) {
    response->success = false;
    response->message = "Unknown class: " + request->class_name;
    RCLCPP_WARN(this->get_logger(), "Unknown class: %s", request->class_name.c_str());
    return;
  }

  target_class_id_ = class_id;
  current_track_id_ = -1;  // Reset current tracking
  response->success = true;
  response->message = "Target class changed to: " + request->class_name;
  RCLCPP_INFO(this->get_logger(), "Target class changed to: %s (class_id=%d)",
              request->class_name.c_str(), class_id);
}

int BehaviorNode::GetClassIdByName(const std::string& class_name) const {
  for (const auto& [id, name] : coco_classes_) {
    if (name == class_name) {
      return id;
    }
  }
  return -1;
}

}  // namespace behavior_node
