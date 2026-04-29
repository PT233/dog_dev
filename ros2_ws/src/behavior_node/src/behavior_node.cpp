#include "behavior_node/behavior_node.hpp"

#include "shared/load_trimmed_lines.hpp"

namespace behavior_node
{

BehaviorNode::BehaviorNode(const rclcpp::NodeOptions & options)
: rclcpp::Node("behavior_node", options)
{
  // 图像尺寸和中心点来自 YAML；像素误差以该中心点为零点。
  this->declare_parameter<int>("image.width", 320);
  this->declare_parameter<int>("image.height", 480);
  this->declare_parameter<int>("image.center_x", 160);
  this->declare_parameter<int>("image.center_y", 240);

  image_width_ = this->get_parameter("image.width").as_int();
  image_height_ = this->get_parameter("image.height").as_int();
  center_x_ = this->get_parameter("image.center_x").as_int();
  center_y_ = this->get_parameter("image.center_y").as_int();

  // 载入 COCO 类别名，服务切换目标类别时用名称查 ID。
  load_coco_classes();

  // 订阅跟踪后的检测结果，输入已经带 track_id。
  auto qos = rclcpp::SensorDataQoS();
  tracked_objects_sub_ =
    this->create_subscription<robot_interfaces::msg::Detection2DArray>(
      "~/input/tracked_objects", qos,
      std::bind(&BehaviorNode::tracked_objects_callback, this, std::placeholders::_1));

  // 输出给视觉伺服节点的像素误差：(目标中心 - 画面中心)。
  pixel_error_pub_ =
    this->create_publisher<geometry_msgs::msg::Vector3>("~/output/pixel_error", rclcpp::QoS(5));

  // 运行中可通过服务把目标从 person 切换到其他 COCO 类别。
  set_target_class_srv_ =
    this->create_service<robot_interfaces::srv::SetTargetClass>(
      "~/input/set_target_class",
      std::bind(&BehaviorNode::set_target_class_callback, this, std::placeholders::_1,
                std::placeholders::_2));

  std::string class_name = "person";
  if (coco_classes_.count(target_class_id_)) {
    class_name = coco_classes_[target_class_id_];
  }
  RCLCPP_INFO(this->get_logger(),
      "BehaviorNode initialized (target: %s, class_id=%d, center: %d,%d)",
              class_name.c_str(), target_class_id_, center_x_, center_y_);
}

void BehaviorNode::tracked_objects_callback(
  const robot_interfaces::msg::Detection2DArray::SharedPtr tracked_objects_msg)
{
  if (!tracked_objects_msg || tracked_objects_msg->detections.empty()) {
    // 没有跟踪结果时不发布误差，控制节点会按超时逻辑停止步态输出。
    RCLCPP_INFO(this->get_logger(), "No target");
    return;
  }

  // 从本帧所有检测中选出当前类别的最佳目标。
  if (select_target(*tracked_objects_msg)) {
    std::string class_name = "unknown";
    if (coco_classes_.count(target_class_id_)) {
      class_name = coco_classes_[target_class_id_];
    }
    RCLCPP_INFO(this->get_logger(),
                "Target: id=%d, center=(%d, %d), class=%s",
                current_track_id_, target_center_x_, target_center_y_, class_name.c_str());

    // 像素误差保持像素单位，控制节点再用 PID 转成角度命令。
    auto pixel_error = geometry_msgs::msg::Vector3();
    pixel_error.x = target_center_x_ - center_x_;
    pixel_error.y = target_center_y_ - center_y_;
    pixel_error.z = 0.0;
    pixel_error_pub_->publish(pixel_error);
  } else {
    RCLCPP_INFO(this->get_logger(), "No target");
  }
}

bool BehaviorNode::select_target(
  const robot_interfaces::msg::Detection2DArray & detection_array)
{
  int best_detection_index = -1;
  float max_detection_area = 0.0f;

  // 策略：从同类别目标中选面积最大的（即最近/最显著的目标）
  // 面积大通常意味着目标距摄像头更近，是最优跟踪候选
  for (size_t i = 0; i < detection_array.detections.size(); ++i) {
    const auto & detection = detection_array.detections[i];

    if (detection.class_id != target_class_id_) {
      continue;
    }

    float detection_area = detection.width * detection.height;
    if (detection_area > max_detection_area) {
      max_detection_area = detection_area;
      best_detection_index = static_cast<int>(i);
    }
  }

  if (best_detection_index < 0) {
    return false;
  }

  const auto & target_detection = detection_array.detections[best_detection_index];
  target_center_x_ = static_cast<int>(target_detection.center_x);
  target_center_y_ = static_cast<int>(target_detection.center_y);
  // track_id 是消息字段中的字符串，行为节点只保存为整数用于日志显示。
  current_track_id_ = std::stoi(target_detection.track_id);

  return true;
}
// 加载 COCO 类别
void BehaviorNode::load_coco_classes()
{
  const std::string coco_classes_file = "models/coco_classes.txt";
  std::vector<std::string> class_names;

  if (!project_shared::load_trimmed_lines(coco_classes_file, &class_names)) {
    RCLCPP_WARN(this->get_logger(), "Could not open COCO classes file: %s",
                coco_classes_file.c_str());
    return;
  }

  for (size_t i = 0; i < class_names.size(); ++i) {
    // COCO 文件行号即 class_id，保持与 YOLO 模型输出一致。
    coco_classes_[static_cast<int>(i)] = class_names[i];
  }

  RCLCPP_DEBUG(this->get_logger(), "Loaded %zu COCO classes", coco_classes_.size());
}
// 服务回调
void BehaviorNode::set_target_class_callback(
  const std::shared_ptr<robot_interfaces::srv::SetTargetClass::Request> request,
  std::shared_ptr<robot_interfaces::srv::SetTargetClass::Response> response)
{
  int class_id = class_id_by_name(request->class_name);

  if (class_id < 0) {
    response->success = false;
    response->message = "Unknown class: " + request->class_name;
    RCLCPP_WARN(this->get_logger(), "Unknown class: %s", request->class_name.c_str());
    return;
  }

  target_class_id_ = class_id;
  current_track_id_ = -1;  // 切换类别后丢弃旧目标 ID，等待下一帧重新选择。
  response->success = true;
  response->message = "Target class changed to: " + request->class_name;
  RCLCPP_INFO(this->get_logger(), "Target class changed to: %s (class_id=%d)",
              request->class_name.c_str(), class_id);
}

int BehaviorNode::class_id_by_name(const std::string & class_name) const
{
  for (const auto & [id, name] : coco_classes_) {
    if (name == class_name) {
      return id;
    }
  }
  return -1;
}

}  // namespace behavior_node
