#include "detection_node/yolo_infer.hpp"

#include "shared/load_trimmed_lines.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numeric>

namespace detection_node
{

namespace
{

constexpr int kModelInputSize = 640;
constexpr int kInputChannels = 3;
constexpr int kCandidateCount = 8400;
constexpr int kOutputDimension = 84;
constexpr float kConfidenceThreshold = 0.25f;
constexpr float kFinalConfidenceThreshold = 0.5f;

}  // namespace

YoloInfer::YoloInfer(
  const std::string & model_path, bool use_cuda,
  int intra_op_threads, int inter_op_threads)
: model_path_(model_path), is_cuda_enabled_(false)
{
    // COCO 类别文件用于确定类别数和后续调试显示；读取失败时按 80 类默认值运行。
  if (project_shared::load_trimmed_lines("models/coco_classes.txt", &class_names_)) {
    num_classes_ = class_names_.size();
  }

  try {
    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "detection");
    memory_info_ = std::make_unique<Ort::MemoryInfo>(
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));

    Ort::SessionOptions session_opts;
        // 推理线程数显式配置，方便在树莓派/WSL/GPU 主机间调性能。
    session_opts.SetIntraOpNumThreads(intra_op_threads);
    session_opts.SetInterOpNumThreads(inter_op_threads);
    session_opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    if (use_cuda) {
      try {
        OrtCUDAProviderOptions cuda_options;
        session_opts.AppendExecutionProvider_CUDA(cuda_options);
        is_cuda_enabled_ = true;
      } catch (const std::exception & e) {
                // CUDA provider 不可用时降级 CPU，避免缺 GPU 的环境直接启动失败。
        is_cuda_enabled_ = false;
      }
    }

    session_ = std::make_unique<Ort::Session>(*env_, model_path_.c_str(), session_opts);

        // 取出模型输入输出名称，并保存字符串所有权，保证 Run() 时 char* 仍有效。
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
  } catch (const std::exception & e) {
    throw std::runtime_error(std::string("Failed to initialize ONNX Runtime: ") + e.what());
  }
}

YoloInfer::~YoloInfer() = default;

LetterboxParams YoloInfer::letterbox(const cv::Mat & image, cv::Mat & letterboxed, int target_size)
{
    // 保持原图宽高比缩放到 target_size 方形画布，剩余区域填 114 灰色。
  int image_height = image.rows;
  int image_width = image.cols;
  float scale = std::min(
        static_cast<float>(target_size) / image_height,
        static_cast<float>(target_size) / image_width);
  int resized_height = image_height * scale;
  int resized_width = image_width * scale;

  cv::Mat resized;
  cv::resize(image, resized, cv::Size(resized_width, resized_height), 0, 0, cv::INTER_LINEAR);

  cv::Mat canvas(target_size, target_size, CV_8UC3, cv::Scalar(114, 114, 114));
  int padding_y = (target_size - resized_height) / 2;
  int padding_x = (target_size - resized_width) / 2;
  resized.copyTo(canvas(cv::Rect(padding_x, padding_y, resized_width, resized_height)));

  letterboxed = canvas;
  return LetterboxParams{scale, padding_x, padding_y};
}

std::vector<Detection> YoloInfer::infer(const cv::Mat & image)
{
    // YOLOv8n 期望 640x640 RGB、float32、NCHW、数值范围 [0,1]。
  cv::Mat letterboxed;
  LetterboxParams params = letterbox(image, letterboxed, kModelInputSize);

  cv::Mat rgb_image;
  cv::cvtColor(letterboxed, rgb_image, cv::COLOR_BGR2RGB);

  cv::Mat float_image;
  rgb_image.convertTo(float_image, CV_32F, 1.0 / 255.0);

  std::vector<float> model_input;
  model_input.reserve(1 * kInputChannels * kModelInputSize * kModelInputSize);

    // HWC → CHW 格式转换：OpenCV 内存布局是 HWC（H×W×C），
    // ONNX Runtime 期望 NCHW（批次×通道×高×宽），需要转置
  for (int channel_index = 0; channel_index < kInputChannels; ++channel_index) {
    for (int row = 0; row < kModelInputSize; ++row) {
      for (int column = 0; column < kModelInputSize; ++column) {
        model_input.push_back(float_image.at<cv::Vec3f>(row, column)[channel_index]);
      }
    }
  }

  std::vector<int64_t> input_shape{1, kInputChannels, kModelInputSize, kModelInputSize};
  std::vector<Ort::Value> input_tensors;
  input_tensors.push_back(Ort::Value::CreateTensor<float>(
        *memory_info_, model_input.data(), model_input.size(),
        input_shape.data(), input_shape.size()));

  auto output_tensors = session_->Run(
        Ort::RunOptions{nullptr},
        input_names_.data(), input_tensors.data(), input_tensors.size(),
        output_names_.data(), output_names_.size());

  const float * output_data = output_tensors[0].GetTensorMutableData<float>();
  std::vector<int64_t> output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();

    // 模型输出 shape: [1, 84, 8400]，转置为 [8400, 84]
    // 原始格式（列优先）不便于逐检测框遍历，转置后每行对应一个候选框
  std::vector<float> outputs(output_shape[1] * output_shape[2]);
  for (int i = 0; i < output_shape[1]; ++i) {
    for (int j = 0; j < output_shape[2]; ++j) {
      outputs[j * output_shape[1] + i] = output_data[i * output_shape[2] + j];
    }
  }

  return post_process(outputs, params);
}

std::vector<Detection> YoloInfer::post_process(
  const std::vector<float> & outputs,
  const LetterboxParams & params)
{
  std::vector<Detection> detections;

  for (int detection_index = 0; detection_index < kCandidateCount; ++detection_index) {
    const float * candidate_data = outputs.data() + detection_index * kOutputDimension;

    float center_x = candidate_data[0];
    float center_y = candidate_data[1];
    float width = candidate_data[2];
    float height = candidate_data[3];
    float object_confidence = candidate_data[4];

    if (object_confidence < kConfidenceThreshold) {continue;}

        // 找到类别概率最大的类别，再与 objectness 相乘得到最终置信度。
    float max_prob = 0.0f;
    int class_id = 0;
    for (int class_index = 5; class_index < kOutputDimension; ++class_index) {
      if (candidate_data[class_index] > max_prob) {
        max_prob = candidate_data[class_index];
        class_id = class_index - 5;
      }
    }

    float confidence = object_confidence * max_prob;
    if (confidence < kFinalConfidenceThreshold) {continue;}

        // de-letterbox：还原 padding 偏移再除以缩放比例，
        // 将模型坐标系（640×640）映射回原始图像坐标系
    float real_center_x = (center_x - params.padding_x) / params.scale;
    float real_center_y = (center_y - params.padding_y) / params.scale;
    float real_width = width / params.scale;
    float real_height = height / params.scale;

    int x_min = std::max(0, static_cast<int>(real_center_x - real_width / 2));
    int y_min = std::max(0, static_cast<int>(real_center_y - real_height / 2));
    int x_max = static_cast<int>(real_center_x + real_width / 2);
    int y_max = static_cast<int>(real_center_y + real_height / 2);

    Detection detection;
    detection.bounding_box = cv::Rect(x_min, y_min, x_max - x_min, y_max - y_min);
    detection.class_id = class_id;
    detection.confidence = confidence;
    detections.push_back(detection);
  }

  return non_maximum_suppression(detections);
}

std::vector<Detection> YoloInfer::non_maximum_suppression(
  const std::vector<Detection> & detections,
  float iou_threshold)
{
  if (detections.empty()) {
    return {};
  }

  // 按置信度从高到低保留框，和已保留框 IoU 过高的候选会被抑制。
  std::vector<Detection> sorted_detections = detections;
  std::sort(sorted_detections.begin(), sorted_detections.end(),
    [](const Detection & a, const Detection & b) {
      return a.confidence > b.confidence;
              });

  std::vector<Detection> results;
  for (const auto & detection : sorted_detections) {
    bool keep = true;
    for (const auto & kept : results) {
      float intersection_area = (detection.bounding_box & kept.bounding_box).area();
      float union_area = detection.bounding_box.area() + kept.bounding_box.area() -
        intersection_area;
      float iou = intersection_area / (union_area + 1e-6f);

      if (iou > iou_threshold) {
        keep = false;
        break;
      }
    }
    if (keep) {results.push_back(detection);}
  }

  return results;
}

}  // namespace detection_node
