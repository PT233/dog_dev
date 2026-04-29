#pragma once

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "robot_interfaces/msg/simple_detection2_d_array.hpp"
#include "detection_node/yolo_infer.hpp"
#include <opencv2/opencv.hpp>
#include <memory>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

// YOLO 检测节点
//
// 订阅 /camera/image_mono，在独立推理线程中执行 ONNX Runtime 推理，
// 发布 /detections。订阅回调只做入队，避免相机线程被深度学习推理阻塞。
class DetectionNode : public rclcpp::Node {
public:
    explicit DetectionNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
    ~DetectionNode();

private:
    // 图像订阅回调：把最新图像放入有限深度队列。
    void ImageCallback(sensor_msgs::msg::Image::UniquePtr msg);

    // 推理工作线程：取队列图像、运行 YOLO、发布检测结果。
    void InferenceWorker();

    std::unique_ptr<detection_node::YoloInfer> infer_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_image_;
    rclcpp::Publisher<robot_interfaces::msg::SimpleDetection2DArray>::SharedPtr pub_detections_;

    std::queue<std::unique_ptr<sensor_msgs::msg::Image>> image_queue_;  // 有限深度推理队列
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::thread inference_thread_;  // 与 ROS executor 分离的推理线程
    bool running_;                  // 析构时通知线程退出
};
