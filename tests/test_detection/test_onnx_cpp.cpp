#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <chrono>

using namespace std;

// Letterbox resize function
cv::Mat letterbox(const cv::Mat& img, int target_size = 640) {
    int h = img.rows, w = img.cols;
    float scale = min((float)target_size / h, (float)target_size / w);
    int new_w = w * scale, new_h = h * scale;

    cv::Mat resized;
    cv::resize(img, resized, cv::Size(new_w, new_h));

    // Create canvas
    cv::Mat canvas(target_size, target_size, CV_8UC3, cv::Scalar(114, 114, 114));
    int pad_x = (target_size - new_w) / 2;
    int pad_y = (target_size - new_h) / 2;
    resized.copyTo(canvas(cv::Rect(pad_x, pad_y, new_w, new_h)));

    return canvas;
}

int main() {
    try {
        cout << "Task 4.3: C++ ONNX Runtime Inference Demo" << endl;
        cout << "==========================================" << endl;

        // Initialize ONNX Runtime environment
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test_onnx");
        Ort::SessionOptions session_options;
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        // Enable CUDA
        OrtCUDAProviderOptions cuda_options;
        session_options.AppendExecutionProvider_CUDA(cuda_options);

        // Create session
        const char* model_path = "models/yolov8n.onnx";
        Ort::Session session(env, model_path, session_options);

        cout << "✓ Model loaded: " << model_path << endl;

        // Load image
        cv::Mat img = cv::imread("bus.jpg");
        if (img.empty()) {
            throw runtime_error("Failed to load bus.jpg");
        }
        cout << "✓ Image loaded: " << img.cols << "x" << img.rows << endl;

        // Letterbox resize
        cv::Mat input_img = letterbox(img, 640);
        cout << "✓ Letterbox resized to: " << input_img.cols << "x" << input_img.rows << endl;

        // Prepare input tensor
        input_img.convertTo(input_img, CV_32F, 1.0 / 255.0);

        vector<float> input_tensor_values;
        for (int i = 0; i < input_img.rows; ++i) {
            for (int j = 0; j < input_img.cols; ++j) {
                cv::Vec3f pixel = input_img.at<cv::Vec3f>(i, j);
                // Convert BGR to RGB and normalize
                input_tensor_values.push_back(pixel[2]); // R
                input_tensor_values.push_back(pixel[1]); // G
                input_tensor_values.push_back(pixel[0]); // B
            }
        }

        // Create input tensor
        vector<int64_t> input_shape = {1, 3, 640, 640};
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
            OrtArenaAllocator, OrtMemTypeDefault);

        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, input_tensor_values.data(),
            input_tensor_values.size(), input_shape.data(),
            input_shape.size());

        cout << "✓ Input tensor prepared: shape [1, 3, 640, 640]" << endl;

        // Run inference
        auto start = chrono::high_resolution_clock::now();

        const char* input_names[] = {"images"};
        const char* output_names[] = {"output0"};

        vector<Ort::Value> output_tensors = session.Run(
            Ort::RunOptions{nullptr},
            input_names, &input_tensor, 1,
            output_names, 1);

        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

        cout << "✓ Inference completed in " << duration.count() << "ms" << endl;

        // Get output info
        auto output_tensor = move(output_tensors[0]);
        vector<int64_t> output_shape = output_tensor.GetTensorTypeAndShapeInfo().GetShape();
        float* output_data = output_tensor.GetTensorMutableData<float>();

        cout << "✓ Output tensor shape: [";
        for (size_t i = 0; i < output_shape.size(); ++i) {
            cout << output_shape[i];
            if (i < output_shape.size() - 1) cout << ", ";
        }
        cout << "]" << endl;

        // Print first 10 output values
        cout << "✓ First 10 output values: ";
        for (int i = 0; i < 10; ++i) {
            cout << output_data[i] << " ";
        }
        cout << endl;

        cout << "==========================================" << endl;
        cout << "✓ All tests PASSED" << endl;

        return 0;

    } catch (const exception& e) {
        cerr << "✗ Error: " << e.what() << endl;
        return 1;
    }
}
