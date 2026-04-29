#include "detection_node/detection_node.hpp"
#include <chrono>

namespace detection_node
{

DetectionNode::DetectionNode(const rclcpp::NodeOptions & options)
: Node("detection_node", options), is_running_(true)
{
    // 参数从 YAML 读取，默认使用 YOLOv8n ONNX 模型和 CUDA 推理。
  declare_parameter<std::string>("inference.model_path", "models/yolov8n.onnx");
  declare_parameter<float>("inference.confidence_threshold", 0.25f);
  declare_parameter<float>("inference.nms_threshold", 0.45f);
  declare_parameter<bool>("inference.use_cuda", true);
  declare_parameter<int>("inference.queue_size", 2);
  declare_parameter<int>("onnx_runtime.intra_threads", 2);
  declare_parameter<int>("onnx_runtime.inter_threads", 1);

  std::string model_path = get_parameter("inference.model_path").as_string();
  float confidence_threshold = get_parameter("inference.confidence_threshold").as_double();
  float nms_threshold = get_parameter("inference.nms_threshold").as_double();
  bool use_cuda = get_parameter("inference.use_cuda").as_bool();
  int intra_threads = get_parameter("onnx_runtime.intra_threads").as_int();
  int inter_threads = get_parameter("onnx_runtime.inter_threads").as_int();

  RCLCPP_INFO(this->get_logger(), "Detection node parameters:");
  RCLCPP_INFO(this->get_logger(), "  inference.model_path: %s", model_path.c_str());
  RCLCPP_INFO(this->get_logger(), "  inference.confidence_threshold: %.2f", confidence_threshold);
  RCLCPP_INFO(this->get_logger(), "  inference.nms_threshold: %.2f", nms_threshold);
  RCLCPP_INFO(this->get_logger(), "  inference.use_cuda: %s", use_cuda ? "true" : "false");
  RCLCPP_INFO(this->get_logger(), "  onnx_runtime.intra_threads: %d", intra_threads);
  RCLCPP_INFO(this->get_logger(), "  onnx_runtime.inter_threads: %d", inter_threads);

  try {
        // YoloInfer 封装 ONNX Runtime session、预处理和后处理。
    yolo_infer_ = std::make_unique<YoloInfer>(
            model_path, use_cuda, intra_threads, inter_threads);
    RCLCPP_INFO(this->get_logger(), "YoloInfer initialized successfully");
    RCLCPP_INFO(this->get_logger(), "CUDA enabled: %s",
                   yolo_infer_->is_cuda_enabled() ? "true" : "false");
  } catch (const std::exception & e) {
    RCLCPP_ERROR(this->get_logger(), "Failed to initialize YoloInfer: %s", e.what());
    throw;
  }

    // 输入图像来自 stereo_splitter，编码应为 bgr8。
  image_sub_ = create_subscription<sensor_msgs::msg::Image>(
        "~/input/image",
        rclcpp::SensorDataQoS(),
        std::bind(&DetectionNode::image_callback, this, std::placeholders::_1));

    // 输出只包含检测框和类别，track_id 由 tracker_node 后续补充。
  detections_pub_ = create_publisher<robot_interfaces::msg::Detection2DArray>(
        "~/output/detections", 10);

    // 推理放在线程中，避免相机订阅回调被模型执行时间阻塞。
  inference_thread_ = std::thread(&DetectionNode::inference_worker, this);

  RCLCPP_INFO(this->get_logger(), "Detection node started successfully");
}

DetectionNode::~DetectionNode()
{
    // 唤醒等待中的推理线程，确保析构时能正常 join。
  is_running_ = false;
  queue_condition_.notify_all();
  if (inference_thread_.joinable()) {
    inference_thread_.join();
  }
}

void DetectionNode::image_callback(sensor_msgs::msg::Image::UniquePtr msg)
{
  if (!is_running_) {return;}

  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
        // 队列限深 2：避免推理线程跟不上时内存无限增长（直接丢弃较旧的帧）
        // 采用 UniquePtr 减少拷贝，图像数据在所有权转移后不再被复制
    if (image_queue_.size() < 2) {
      image_queue_.push(std::move(msg));
      queue_condition_.notify_one();
    }
  }
}

void DetectionNode::inference_worker()
{
  while (is_running_) {
    std::unique_ptr<sensor_msgs::msg::Image> image_msg;

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      queue_condition_.wait(lock, [this] {return !image_queue_.empty() || !is_running_;});

      if (!is_running_) {break;}

      if (image_queue_.empty()) {continue;}

      image_msg = std::move(image_queue_.front());
      image_queue_.pop();
    }

    if (!image_msg) {continue;}

    try {
      auto infer_start = std::chrono::high_resolution_clock::now();

            // 这里复用 ROS Image 内部数据创建 Mat 视图，不额外拷贝像素。
      cv::Mat image(image_msg->height, image_msg->width, CV_8UC3,
        image_msg->data.data(), image_msg->step);

      auto detections = yolo_infer_->infer(image);

      auto infer_end = std::chrono::high_resolution_clock::now();
      auto infer_time_ms = std::chrono::duration<double, std::milli>(
                infer_end - infer_start).count();

      auto detection_array = std::make_unique<robot_interfaces::msg::Detection2DArray>();
      detection_array->header.stamp = image_msg->header.stamp;
      detection_array->header.frame_id = image_msg->header.frame_id;
      detection_array->detections.reserve(detections.size());

      for (const auto & det : detections) {
                // OpenCV Rect 是左上角格式，项目消息统一使用中心点 + 宽高格式。
        robot_interfaces::msg::Detection2D detection_msg;
        detection_msg.center_x = det.bounding_box.x + det.bounding_box.width / 2.0;
        detection_msg.center_y = det.bounding_box.y + det.bounding_box.height / 2.0;
        detection_msg.width = det.bounding_box.width;
        detection_msg.height = det.bounding_box.height;
        detection_msg.class_id = det.class_id;
        detection_msg.confidence = det.confidence;
        detection_msg.track_id = "";
        detection_array->detections.push_back(detection_msg);
      }

      detections_pub_->publish(std::move(detection_array));

      RCLCPP_DEBUG(this->get_logger(),
                       "Inference time: %.2f ms, detections: %lu",
                       infer_time_ms, detections.size());
    } catch (const std::exception & e) {
      RCLCPP_ERROR(this->get_logger(), "Inference error: %s", e.what());
    }
  }
}

}  // namespace detection_node
