#ifndef DETECTION_NODE__DETECTION_NODE_HPP_
#define DETECTION_NODE__DETECTION_NODE_HPP_

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "robot_interfaces/msg/detection2_d_array.hpp"
#include "detection_node/yolo_infer.hpp"
#include <opencv2/opencv.hpp>
#include <memory>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

// YOLO 检测节点
//
// 订阅私有图像输入，在独立推理线程中执行 ONNX Runtime 推理，
// 发布私有检测输出。订阅回调只做入队，避免相机线程被深度学习推理阻塞。
namespace detection_node
{

class DetectionNode : public rclcpp::Node {
public:
  explicit DetectionNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  ~DetectionNode();

private:
    // 图像订阅回调：把最新图像放入有限深度队列。
  void image_callback(sensor_msgs::msg::Image::UniquePtr msg);

    // 推理工作线程：取队列图像、运行 YOLO、发布检测结果。
  void inference_worker();

  std::unique_ptr<YoloInfer> yolo_infer_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
  rclcpp::Publisher<robot_interfaces::msg::Detection2DArray>::SharedPtr detections_pub_;

  std::queue<std::unique_ptr<sensor_msgs::msg::Image>> image_queue_;    // 有限深度推理队列
  std::mutex queue_mutex_;
  std::condition_variable queue_condition_;
  std::thread inference_thread_;    // 与 ROS executor 分离的推理线程
  bool is_running_;                 // 析构时通知线程退出
};

}  // namespace detection_node

#endif  // DETECTION_NODE__DETECTION_NODE_HPP_
