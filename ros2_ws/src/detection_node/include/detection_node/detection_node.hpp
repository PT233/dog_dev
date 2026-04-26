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

class DetectionNode : public rclcpp::Node {
public:
    explicit DetectionNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
    ~DetectionNode();

private:
    void ImageCallback(sensor_msgs::msg::Image::UniquePtr msg);
    void InferenceWorker();

    std::unique_ptr<detection_node::YoloInfer> infer_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_image_;
    rclcpp::Publisher<robot_interfaces::msg::SimpleDetection2DArray>::SharedPtr pub_detections_;

    std::queue<std::unique_ptr<sensor_msgs::msg::Image>> image_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::thread inference_thread_;
    bool running_;
};
