#pragma once

#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

namespace detection_node {

struct Detection {
    cv::Rect bbox;
    int class_id;
    float confidence;
};

struct LetterboxParams {
    float scale;
    int pad_x;
    int pad_y;
};

class YoloInfer {
public:
    YoloInfer(const std::string& model_path, bool use_cuda = false,
              int intra_op_threads = 1, int inter_op_threads = 1);
    ~YoloInfer();

    std::vector<Detection> Infer(const cv::Mat& image);
    bool IsCudaEnabled() const { return cuda_enabled_; }
    int GetNumClasses() const { return num_classes_; }
    const std::vector<std::string>& GetClassNames() const { return class_names_; }

private:
    std::string model_path_;
    bool cuda_enabled_;
    int num_classes_ = 80;
    std::vector<std::string> class_names_;

    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::unique_ptr<Ort::MemoryInfo> memory_info_;

    std::vector<const char*> input_names_;
    std::vector<const char*> output_names_;
    std::vector<std::string> input_names_storage_;
    std::vector<std::string> output_names_storage_;

    LetterboxParams Letterbox(const cv::Mat& img, cv::Mat& letterboxed, int target_size = 640);
    std::vector<Detection> PostProcess(const std::vector<float>& outputs, const LetterboxParams& params);
    std::vector<Detection> NMS(const std::vector<Detection>& detections, float iou_threshold = 0.45);
};

}  // namespace detection_node
