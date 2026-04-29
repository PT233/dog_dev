#ifndef BEHAVIOR_NODE__BEHAVIOR_NODE_HPP_
#define BEHAVIOR_NODE__BEHAVIOR_NODE_HPP_

// 行为决策节点
// 从跟踪结果中按类别筛选目标，选择面积最大的候选，
// 计算其相对画面中心的像素偏差并发布，供腿部控制节点生成步态命令。
// 支持通过私有服务运行时切换跟踪目标类别。

#include "rclcpp/rclcpp.hpp"
#include "robot_interfaces/msg/detection2_d_array.hpp"
#include "robot_interfaces/srv/set_target_class.hpp"
#include "geometry_msgs/msg/vector3.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace behavior_node
{

// 行为决策节点
// 订阅：私有 tracked_objects 输入（来自 tracker_node）
// 发布：私有 pixel_error 输出 (Vector3) = (target_center_x - center_x, target_center_y - center_y, 0)
// 服务：私有 set_target_class 输入，动态切换跟踪的 COCO 类别
class BehaviorNode : public rclcpp::Node {
public:
  BehaviorNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  rclcpp::Subscription<robot_interfaces::msg::Detection2DArray>::SharedPtr tracked_objects_sub_;
  rclcpp::Publisher<geometry_msgs::msg::Vector3>::SharedPtr pixel_error_pub_;
  rclcpp::Service<robot_interfaces::srv::SetTargetClass>::SharedPtr set_target_class_srv_;

  // 当前跟踪目标类别（COCO class_id，默认 0 = person）
  int target_class_id_ = 0;
  int current_track_id_ = -1;  // 当前锁定的 track_id（-1 表示未锁定）
  int target_center_x_ = 0;    // 目标中心像素 X（从最近一帧更新）
  int target_center_y_ = 0;    // 目标中心像素 Y

  // 图像参数（从 YAML 加载）
  int image_width_ = 320;
  int image_height_ = 480;
  int center_x_ = 160;   // 画面中心 X
  int center_y_ = 240;   // 画面中心 Y

  // COCO 类别 ID → 名称映射（从 coco_classes.txt 加载）
  std::map<int, std::string> coco_classes_;

  // ROS 回调：处理跟踪结果和动态切换目标类别服务。
  void tracked_objects_callback(
    const robot_interfaces::msg::Detection2DArray::SharedPtr tracked_objects_msg);
  void set_target_class_callback(
    const std::shared_ptr<robot_interfaces::srv::SetTargetClass::Request> request,
    std::shared_ptr<robot_interfaces::srv::SetTargetClass::Response> response);

  // 内部辅助函数：目标筛选、COCO 类别表加载和类别名反查。
  bool select_target(const robot_interfaces::msg::Detection2DArray & detection_array);
  void load_coco_classes();
  int class_id_by_name(const std::string & class_name) const;
};

}  // namespace behavior_node

#endif  // BEHAVIOR_NODE__BEHAVIOR_NODE_HPP_
