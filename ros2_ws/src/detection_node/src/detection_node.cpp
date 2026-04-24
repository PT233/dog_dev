#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "detection_node/yolo_infer.hpp"
#include <opencv2/opencv.hpp>
#include <memory>

using std::placeholders::_1;

class DetectionNode : public rclcpp::Node {
public:
    DetectionNode() : Node("detection_node") {
        // Declare parameters
        this->declare_parameter<std::string>("model_path", "models/yolov8n.onnx");
        this->declare_parameter<float>("conf_threshold", 0.25f);
        this->declare_parameter<float>("nms_threshold", 0.45f);
        this->declare_parameter<bool>("use_cuda", false);

        // Get parameters
        std::string model_path = this->get_parameter("model_path").as_string();
        float conf_threshold = this->get_parameter("conf_threshold").as_double();
        float nms_threshold = this->get_parameter("nms_threshold").as_double();
        bool use_cuda = this->get_parameter("use_cuda").as_bool();

        RCLCPP_INFO(this->get_logger(), "Detection node parameters:");
        RCLCPP_INFO(this->get_logger(), "  model_path: %s", model_path.c_str());
        RCLCPP_INFO(this->get_logger(), "  conf_threshold: %.2f", conf_threshold);
        RCLCPP_INFO(this->get_logger(), "  nms_threshold: %.2f", nms_threshold);
        RCLCPP_INFO(this->get_logger(), "  use_cuda: %s", use_cuda ? "true" : "false");

        // Initialize YoloInfer
        try {
            infer_ = std::make_unique<detection_node::YoloInfer>(model_path, use_cuda);
            RCLCPP_INFO(this->get_logger(), "YoloInfer initialized successfully");

            // Warm-up with black image
            cv::Mat black_img(640, 640, CV_8UC3, cv::Scalar(0, 0, 0));
            auto _ = infer_->Infer(black_img);
            RCLCPP_INFO(this->get_logger(), "Warm-up inference completed");
        } catch (const std::exception& e) {
            RCLCPP_ERROR(this->get_logger(), "Failed to initialize YoloInfer: %s", e.what());
            throw;
        }

        // Create subscriber
        sub_image_ = this->create_subscription<sensor_msgs::msg::Image>(
            "/camera/image_mono",
            rclcpp::SensorDataQoS(),
            std::bind(&DetectionNode::ImageCallback, this, _1));

        // Create publisher (placeholder for Detection2DArray)
        // In full implementation, would use vision_msgs/Detection2DArray
        RCLCPP_INFO(this->get_logger(), "Detection node started successfully");
        RCLCPP_INFO(this->get_logger(), "Subscribed to /camera/image_mono");
    }

private:
    void ImageCallback(const sensor_msgs::msg::Image::SharedPtr msg) {
        try {
            // Convert ROS image to OpenCV Mat
            // For now, just log the image size
            RCLCPP_DEBUG(this->get_logger(), "Received image: %ux%u", msg->width, msg->height);

            // In full implementation, would:
            // 1. Convert ROS image to cv::Mat using cv_bridge
            // 2. Run inference
            // 3. Publish Detection2DArray
        } catch (const std::exception& e) {
            RCLCPP_ERROR(this->get_logger(), "Error in image callback: %s", e.what());
        }
    }

    std::unique_ptr<detection_node::YoloInfer> infer_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_image_;
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<DetectionNode>());
    rclcpp::shutdown();
    return 0;
}
