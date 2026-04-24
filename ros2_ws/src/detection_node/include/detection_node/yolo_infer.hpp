#pragma once

#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>

namespace detection_node {

struct Detection {
    cv::Rect bbox;
    int class_id;
    float confidence;
};

class YoloInfer {
public:
    YoloInfer(const std::string& model_path, bool use_cuda = false);
    ~YoloInfer() = default;

    // Inference
    std::vector<Detection> Infer(const cv::Mat& image);

    // Utility
    int GetNumClasses() const { return num_classes_; }
    const std::vector<std::string>& GetClassNames() const { return class_names_; }

private:
    std::string model_path_;
    bool use_cuda_;
    int num_classes_ = 80;
    std::vector<std::string> class_names_;

    // Preprocessing
    cv::Mat Letterbox(const cv::Mat& img, int target_size = 640);

    // Postprocessing
    std::vector<Detection> PostProcess(const std::vector<float>& outputs);
    std::vector<Detection> NMS(const std::vector<Detection>& detections, float iou_threshold = 0.45);
};

}  // namespace detection_node
