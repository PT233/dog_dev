#include "detection_node/detection_node.hpp"
#include <chrono>

DetectionNode::DetectionNode(const rclcpp::NodeOptions& options)
    : Node("detection_node", options), running_(true) {
    declare_parameter<std::string>("model_path", "models/yolov8n.onnx");
    declare_parameter<float>("conf_threshold", 0.25f);
    declare_parameter<float>("nms_threshold", 0.45f);
    declare_parameter<bool>("use_cuda", true);
    declare_parameter<int>("infer_queue_size", 2);
    declare_parameter<int>("ort_intra_threads", 2);
    declare_parameter<int>("ort_inter_threads", 1);

    std::string model_path = get_parameter("model_path").as_string();
    float conf_threshold = get_parameter("conf_threshold").as_double();
    float nms_threshold = get_parameter("nms_threshold").as_double();
    bool use_cuda = get_parameter("use_cuda").as_bool();
    int intra_threads = get_parameter("ort_intra_threads").as_int();
    int inter_threads = get_parameter("ort_inter_threads").as_int();

    RCLCPP_INFO(this->get_logger(), "Detection node parameters:");
    RCLCPP_INFO(this->get_logger(), "  model_path: %s", model_path.c_str());
    RCLCPP_INFO(this->get_logger(), "  conf_threshold: %.2f", conf_threshold);
    RCLCPP_INFO(this->get_logger(), "  nms_threshold: %.2f", nms_threshold);
    RCLCPP_INFO(this->get_logger(), "  use_cuda: %s", use_cuda ? "true" : "false");
    RCLCPP_INFO(this->get_logger(), "  ort_intra_threads: %d", intra_threads);
    RCLCPP_INFO(this->get_logger(), "  ort_inter_threads: %d", inter_threads);

    try {
        infer_ = std::make_unique<detection_node::YoloInfer>(
            model_path, use_cuda, intra_threads, inter_threads);
        RCLCPP_INFO(this->get_logger(), "YoloInfer initialized successfully");
        RCLCPP_INFO(this->get_logger(), "CUDA enabled: %s",
                   infer_->IsCudaEnabled() ? "true" : "false");
    } catch (const std::exception& e) {
        RCLCPP_ERROR(this->get_logger(), "Failed to initialize YoloInfer: %s", e.what());
        throw;
    }

    sub_image_ = create_subscription<sensor_msgs::msg::Image>(
        "/camera/image_mono",
        rclcpp::SensorDataQoS(),
        std::bind(&DetectionNode::ImageCallback, this, std::placeholders::_1));

    pub_detections_ = create_publisher<robot_interfaces::msg::SimpleDetection2DArray>(
        "/detections", 10);

    inference_thread_ = std::thread(&DetectionNode::InferenceWorker, this);

    RCLCPP_INFO(this->get_logger(), "Detection node started successfully");
}

DetectionNode::~DetectionNode() {
    running_ = false;
    cv_.notify_all();
    if (inference_thread_.joinable()) {
        inference_thread_.join();
    }
}

void DetectionNode::ImageCallback(sensor_msgs::msg::Image::UniquePtr msg) {
    if (!running_) return;

    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        // 队列限深 2：避免推理线程跟不上时内存无限增长（直接丢弃较旧的帧）
        // 采用 UniquePtr 减少拷贝，图像数据在所有权转移后不再被复制
        if (image_queue_.size() < 2) {
            image_queue_.push(std::move(msg));
            cv_.notify_one();
        }
    }
}

void DetectionNode::InferenceWorker() {
    while (running_) {
        std::unique_ptr<sensor_msgs::msg::Image> image_msg;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this] { return !image_queue_.empty() || !running_; });

            if (!running_) break;

            if (image_queue_.empty()) continue;

            image_msg = std::move(image_queue_.front());
            image_queue_.pop();
        }

        if (!image_msg) continue;

        try {
            auto infer_start = std::chrono::high_resolution_clock::now();

            cv::Mat img(image_msg->height, image_msg->width, CV_8UC3,
                       image_msg->data.data(), image_msg->step);

            auto detections = infer_->Infer(img);

            auto infer_end = std::chrono::high_resolution_clock::now();
            auto infer_time_ms = std::chrono::duration<double, std::milli>(
                infer_end - infer_start).count();

            auto detection_array = std::make_unique<robot_interfaces::msg::SimpleDetection2DArray>();
            detection_array->header.stamp = image_msg->header.stamp;
            detection_array->header.frame_id = image_msg->header.frame_id;
            detection_array->detections.reserve(detections.size());

            for (const auto& det : detections) {
                robot_interfaces::msg::SimpleDetection d;
                d.center_x = det.bbox.x + det.bbox.width / 2.0;
                d.center_y = det.bbox.y + det.bbox.height / 2.0;
                d.width = det.bbox.width;
                d.height = det.bbox.height;
                d.class_id = det.class_id;
                d.confidence = det.confidence;
                d.track_id = "";
                detection_array->detections.push_back(d);
            }

            pub_detections_->publish(std::move(detection_array));

            RCLCPP_DEBUG(this->get_logger(),
                       "Inference time: %.2f ms, detections: %lu",
                       infer_time_ms, detections.size());
        } catch (const std::exception& e) {
            RCLCPP_ERROR(this->get_logger(), "Inference error: %s", e.what());
        }
    }
}
