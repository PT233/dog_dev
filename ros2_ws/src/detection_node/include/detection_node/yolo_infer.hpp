#ifndef DETECTION_NODE__YOLO_INFER_HPP_
#define DETECTION_NODE__YOLO_INFER_HPP_

// YOLOv8 ONNX Runtime 推理封装
// 负责图像预处理（letterbox）、模型推理和后处理（NMS、坐标还原）。
// 支持 CUDAExecutionProvider（GPU）和 CPU 推理，优先尝试 CUDA。

#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

namespace detection_node
{

// 单个检测结果（包含 OpenCV 矩形框、类别 ID 和置信度）
struct Detection
{
  cv::Rect bounding_box;
  int class_id;
  float confidence;    // objectness × max_class_prob
};

// Letterbox 预处理参数（保持宽高比缩放时的缩放比例和 padding 量）
// 后处理还原坐标时使用：real_x = (pred_x - padding_x) / scale
struct LetterboxParams
{
  float scale;    // 缩放比例（原始尺寸 × scale = 模型输入尺寸）
  int padding_x;      // 水平 padding（像素）
  int padding_y;      // 垂直 padding（像素）
};

// YOLOv8n 推理器（ONNX Runtime）
// 模型输入：640×640×3 (CHW, float32, 归一化至 [0,1])
// 模型输出：[1, 84, 8400]（84 = 4 坐标 + 80 类别分数）
class YoloInfer {
public:
    // model_path: ONNX 模型文件路径（如 "models/yolov8n.onnx"）
    // use_cuda: 是否尝试启用 CUDAExecutionProvider
    // intra/inter_op_threads: ONNX Runtime 线程数配置
  YoloInfer(
    const std::string & model_path, bool use_cuda = false,
    int intra_op_threads = 1, int inter_op_threads = 1);
  ~YoloInfer();

    // 对输入 BGR 图像执行推理，返回过滤后的检测列表（已做 NMS）
  std::vector<Detection> infer(const cv::Mat & image);
  bool is_cuda_enabled() const {return is_cuda_enabled_;}
  int num_classes() const {return num_classes_;}
  const std::vector<std::string> & class_names() const {return class_names_;}

private:
  std::string model_path_;
  bool is_cuda_enabled_;
  int num_classes_ = 80;
  std::vector<std::string> class_names_;

  std::unique_ptr<Ort::Env> env_;
  std::unique_ptr<Ort::Session> session_;
  std::unique_ptr<Ort::MemoryInfo> memory_info_;

  std::vector<const char *> input_names_;
  std::vector<const char *> output_names_;
  std::vector<std::string> input_names_storage_;
  std::vector<std::string> output_names_storage_;

  LetterboxParams letterbox(const cv::Mat & image, cv::Mat & letterboxed, int target_size = 640);
  std::vector<Detection> post_process(
    const std::vector<float> & outputs, const LetterboxParams & params);
  std::vector<Detection> non_maximum_suppression(
    const std::vector<Detection> & detections, float iou_threshold = 0.45);
};

}  // namespace detection_node

#endif  // DETECTION_NODE__YOLO_INFER_HPP_
