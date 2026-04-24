#include "detection_node/yolo_infer.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

namespace detection_node {

YoloInfer::YoloInfer(const std::string& model_path, bool use_cuda)
    : model_path_(model_path), use_cuda_(use_cuda) {
    // Load class names from coco_classes.txt
    std::ifstream file("models/coco_classes.txt");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                class_names_.push_back(line);
            }
        }
        file.close();
        num_classes_ = class_names_.size();
    }

    // Note: Actual ONNX Runtime initialization would happen here
    // For now, this is a placeholder implementation
}

cv::Mat YoloInfer::Letterbox(const cv::Mat& img, int target_size) {
    int h = img.rows, w = img.cols;
    float scale = std::min((float)target_size / h, (float)target_size / w);
    int new_h = h * scale, new_w = w * scale;

    cv::Mat resized;
    cv::resize(img, resized, cv::Size(new_w, new_h));

    // Create canvas with padding
    cv::Mat canvas(target_size, target_size, CV_8UC3, cv::Scalar(114, 114, 114));
    int pad_y = (target_size - new_h) / 2;
    int pad_x = (target_size - new_w) / 2;
    resized.copyTo(canvas(cv::Rect(pad_x, pad_y, new_w, new_h)));

    return canvas;
}

std::vector<Detection> YoloInfer::PostProcess(const std::vector<float>& outputs) {
    // outputs shape: (1, 84, 8400)
    // Reshape to (8400, 84) where each row is [x, y, w, h, conf, class0, class1, ...]
    std::vector<Detection> detections;

    const int num_detections = 8400;
    const int output_dim = 84;
    const float conf_threshold = 0.25f;

    for (int i = 0; i < num_detections; ++i) {
        float conf = outputs[i * output_dim + 4];  // confidence
        if (conf < conf_threshold) continue;

        // Find max class probability
        float max_prob = 0.0f;
        int class_id = 0;
        for (int j = 5; j < output_dim; ++j) {
            float prob = outputs[i * output_dim + j];
            if (prob > max_prob) {
                max_prob = prob;
                class_id = j - 5;
            }
        }

        float confidence = conf * max_prob;
        if (confidence < 0.5f) continue;  // Skip low confidence detections

        // Get bbox coordinates (normalized)
        float x = outputs[i * output_dim + 0];
        float y = outputs[i * output_dim + 1];
        float w = outputs[i * output_dim + 2];
        float h = outputs[i * output_dim + 3];

        // Convert to pixel coordinates
        int x1 = std::max(0, (int)(x - w / 2));
        int y1 = std::max(0, (int)(y - h / 2));
        int x2 = std::min(640, (int)(x + w / 2));
        int y2 = std::min(640, (int)(y + h / 2));

        Detection det;
        det.bbox = cv::Rect(x1, y1, x2 - x1, y2 - y1);
        det.class_id = class_id;
        det.confidence = confidence;

        detections.push_back(det);
    }

    return NMS(detections);
}

std::vector<Detection> YoloInfer::NMS(const std::vector<Detection>& detections, float iou_threshold) {
    if (detections.empty()) return {};

    // Sort by confidence
    std::vector<Detection> sorted_dets = detections;
    std::sort(sorted_dets.begin(), sorted_dets.end(),
              [](const Detection& a, const Detection& b) {
                  return a.confidence > b.confidence;
              });

    std::vector<Detection> results;
    for (const auto& det : sorted_dets) {
        bool keep = true;
        for (const auto& kept : results) {
            // Calculate IoU
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

std::vector<Detection> YoloInfer::Infer(const cv::Mat& image) {
    // Letterbox resize
    cv::Mat input_img = Letterbox(image, 640);

    // Normalize to [0, 1]
    input_img.convertTo(input_img, CV_32F, 1.0 / 255.0);

    // In a full implementation, we would:
    // 1. Convert BGR to RGB
    // 2. Transpose to CHW format
    // 3. Run ONNX inference
    // 4. Post-process outputs

    // For now, return empty (placeholder)
    std::vector<Detection> detections;

    return detections;
}

}  // namespace detection_node
