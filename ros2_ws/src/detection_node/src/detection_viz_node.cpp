#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include <opencv2/opencv.hpp>
#include <memory>

using std::placeholders::_1;

class DetectionVizNode : public rclcpp::Node {
public:
    DetectionVizNode() : Node("detection_viz_node") {
        // Create subscribers
        sub_image_ = this->create_subscription<sensor_msgs::msg::Image>(
            "/camera/image_mono",
            rclcpp::SensorDataQoS(),
            std::bind(&DetectionVizNode::ImageCallback, this, _1));

        // Create publisher for visualization
        pub_image_ = this->create_publisher<sensor_msgs::msg::Image>(
            "/camera/image_detected",
            rclcpp::SensorDataQoS());

        RCLCPP_INFO(this->get_logger(), "Detection viz node started");
        RCLCPP_INFO(this->get_logger(), "Subscribed to /camera/image_mono");
        RCLCPP_INFO(this->get_logger(), "Publishing to /camera/image_detected");
    }

private:
    void ImageCallback(const sensor_msgs::msg::Image::SharedPtr msg) {
        try {
            // For now, just republish the image as visualization
            // In full implementation, would:
            // 1. Convert to cv::Mat
            // 2. Get synchronized detection results
            // 3. Draw bboxes on image
            // 4. Publish annotated image

            RCLCPP_DEBUG(this->get_logger(), "Received image: %ux%u", msg->width, msg->height);

            // Republish as placeholder
            pub_image_->publish(*msg);
        } catch (const std::exception& e) {
            RCLCPP_ERROR(this->get_logger(), "Error in image callback: %s", e.what());
        }
    }

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_image_;
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_image_;
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<DetectionVizNode>());
    rclcpp::shutdown();
    return 0;
}
