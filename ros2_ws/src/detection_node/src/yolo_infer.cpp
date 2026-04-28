#include "detection_node/yolo_infer.hpp"

#include "shared/load_trimmed_lines.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>

namespace detection_node {

YoloInfer::YoloInfer(const std::string& model_path, bool use_cuda,
                     int intra_op_threads, int inter_op_threads)
    : model_path_(model_path), cuda_enabled_(false) {
    if (project_shared::load_trimmed_lines("models/coco_classes.txt", &class_names_)) {
        num_classes_ = class_names_.size();
    }

    try {
        env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "detection");
        memory_info_ = std::make_unique<Ort::MemoryInfo>(
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));

        Ort::SessionOptions session_opts;
        session_opts.SetIntraOpNumThreads(intra_op_threads);
        session_opts.SetInterOpNumThreads(inter_op_threads);
        session_opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        if (use_cuda) {
            try {
                OrtCUDAProviderOptions cuda_options;
                session_opts.AppendExecutionProvider_CUDA(cuda_options);
                cuda_enabled_ = true;
            } catch (const std::exception& e) {
                cuda_enabled_ = false;
            }
        }

        session_ = std::make_unique<Ort::Session>(*env_, model_path_.c_str(), session_opts);

        // Get input/output info
        size_t num_input_nodes = session_->GetInputCount();
        size_t num_output_nodes = session_->GetOutputCount();

        input_names_storage_.clear();
        output_names_storage_.clear();
        input_names_.clear();
        output_names_.clear();

        for (size_t i = 0; i < num_input_nodes; ++i) {
            auto input_name = session_->GetInputNameAllocated(i, Ort::AllocatorWithDefaultOptions());
            input_names_storage_.push_back(input_name.get());
            input_names_.push_back(input_names_storage_.back().c_str());
        }

        for (size_t i = 0; i < num_output_nodes; ++i) {
            auto output_name = session_->GetOutputNameAllocated(i, Ort::AllocatorWithDefaultOptions());
            output_names_storage_.push_back(output_name.get());
            output_names_.push_back(output_names_storage_.back().c_str());
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to initialize ONNX Runtime: ") + e.what());
    }
}

YoloInfer::~YoloInfer() = default;

LetterboxParams YoloInfer::Letterbox(const cv::Mat& img, cv::Mat& letterboxed, int target_size) {
    int h = img.rows, w = img.cols;
    float scale = std::min((float)target_size / h, (float)target_size / w);
    int new_h = h * scale, new_w = w * scale;

    cv::Mat resized;
    cv::resize(img, resized, cv::Size(new_w, new_h), 0, 0, cv::INTER_LINEAR);

    cv::Mat canvas(target_size, target_size, CV_8UC3, cv::Scalar(114, 114, 114));
    int pad_y = (target_size - new_h) / 2;
    int pad_x = (target_size - new_w) / 2;
    resized.copyTo(canvas(cv::Rect(pad_x, pad_y, new_w, new_h)));

    letterboxed = canvas;
    return LetterboxParams{scale, pad_x, pad_y};
}

std::vector<Detection> YoloInfer::Infer(const cv::Mat& image) {
    cv::Mat letterboxed;
    LetterboxParams params = Letterbox(image, letterboxed, 640);

    cv::Mat rgb_img;
    cv::cvtColor(letterboxed, rgb_img, cv::COLOR_BGR2RGB);

    cv::Mat float_img;
    rgb_img.convertTo(float_img, CV_32F, 1.0 / 255.0);

    std::vector<float> model_input;
    model_input.reserve(1 * 3 * 640 * 640);

    // HWC → CHW 格式转换：OpenCV 内存布局是 HWC（H×W×C），
    // ONNX Runtime 期望 NCHW（批次×通道×高×宽），需要转置
    const int height = 640, width = 640, channels = 3;
    for (int c = 0; c < channels; ++c) {
        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                model_input.push_back(float_img.at<cv::Vec3f>(h, w)[c]);
            }
        }
    }

    std::vector<int64_t> input_shape{1, 3, 640, 640};
    std::vector<Ort::Value> input_tensors;
    input_tensors.push_back(Ort::Value::CreateTensor<float>(
        *memory_info_, model_input.data(), model_input.size(),
        input_shape.data(), input_shape.size()));

    auto output_tensors = session_->Run(
        Ort::RunOptions{nullptr},
        input_names_.data(), input_tensors.data(), input_tensors.size(),
        output_names_.data(), output_names_.size());

    const float* output_data = output_tensors[0].GetTensorMutableData<float>();
    std::vector<int64_t> output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();

    // 模型输出 shape: [1, 84, 8400]，转置为 [8400, 84]
    // 原始格式（列优先）不便于逐检测框遍历，转置后每行对应一个候选框
    std::vector<float> outputs(output_shape[1] * output_shape[2]);
    for (int i = 0; i < output_shape[1]; ++i) {
        for (int j = 0; j < output_shape[2]; ++j) {
            outputs[j * output_shape[1] + i] = output_data[i * output_shape[2] + j];
        }
    }

    return PostProcess(outputs, params);
}

std::vector<Detection> YoloInfer::PostProcess(const std::vector<float>& outputs,
                                             const LetterboxParams& params) {
    std::vector<Detection> detections;

    const int num_detections = 8400;
    const int output_dim = 84;
    const float conf_threshold = 0.25f;

    for (int i = 0; i < num_detections; ++i) {
        const float* data = outputs.data() + i * output_dim;

        float cx = data[0], cy = data[1];
        float w = data[2], h = data[3];
        float conf = data[4];

        if (conf < conf_threshold) continue;

        // Find max class probability
        float max_prob = 0.0f;
        int class_id = 0;
        for (int j = 5; j < output_dim; ++j) {
            if (data[j] > max_prob) {
                max_prob = data[j];
                class_id = j - 5;
            }
        }

        float confidence = conf * max_prob;
        if (confidence < 0.5f) continue;

        // de-letterbox：还原 padding 偏移再除以缩放比例，
        // 将模型坐标系（640×640）映射回原始图像坐标系
        float real_cx = (cx - params.pad_x) / params.scale;
        float real_cy = (cy - params.pad_y) / params.scale;
        float real_w = w / params.scale;
        float real_h = h / params.scale;

        int x1 = std::max(0, (int)(real_cx - real_w / 2));
        int y1 = std::max(0, (int)(real_cy - real_h / 2));
        int x2 = (int)(real_cx + real_w / 2);
        int y2 = (int)(real_cy + real_h / 2);

        Detection det;
        det.bbox = cv::Rect(x1, y1, x2 - x1, y2 - y1);
        det.class_id = class_id;
        det.confidence = confidence;
        detections.push_back(det);
    }

    return NMS(detections);
}

std::vector<Detection> YoloInfer::NMS(const std::vector<Detection>& detections,
                                     float iou_threshold) {
    if (detections.empty()) return {};

    std::vector<Detection> sorted_dets = detections;
    std::sort(sorted_dets.begin(), sorted_dets.end(),
              [](const Detection& a, const Detection& b) {
                  return a.confidence > b.confidence;
              });

    std::vector<Detection> results;
    for (const auto& det : sorted_dets) {
        bool keep = true;
        for (const auto& kept : results) {
            float inter_area = (det.bbox & kept.bbox).area();
            float union_area = det.bbox.area() + kept.bbox.area() - inter_area;
            float iou = inter_area / (union_area + 1e-6f);

            if (iou > iou_threshold) {
                keep = false;
                break;
            }
        }
        if (keep) results.push_back(det);
    }

    return results;
}

}  // namespace detection_node
