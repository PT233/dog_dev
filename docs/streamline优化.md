 视频流三方向性能优化计划

 Context

 机器狗项目的视频处理管道存在三个性能问题：
 1. WSL2 端用 avdec_h264 软件解码，未利用 RTX 4060 的 NVDEC 硬件单元
 2. gst_receiver→stereo_splitter→detection 链路有 3 次不必要的深拷贝（921600B + 921600B + 460800B）
 3. detection_node 的 ImageCallback 是 placeholder，实际推理未实现，且单线程会阻塞接收回调

 优先级与依赖关系

 P1（独立）: 方向1 - gst_receiver 硬件解码
 P2（需先装 ONNX Runtime GPU SDK）: 方向3 - detection_node 完整实现
 P3（依赖 P2 完成，可选）: 方向2 - intra-process 零拷贝

 ---
 方向1：硬件解码（P1）

 目标

 优先用 nvh264dec（NVDEC）解码，失败时自动降级到多线程 avdec_h264。
 管道末尾固定输出 video/x-raw,format=BGR，消除 I420 条件判断分支。

 前置检查

 WSL2 下 nvcodec 插件需要 libnvcuvid.so（在 /usr/lib/wsl/lib/）。
 运行 gst-inspect-1.0 nvh264dec 确认插件是否可用。

 修改 gst_receiver/include/gst_receiver/gst_receiver_node.hpp

 新增：
 private:
   bool try_build_pipeline(bool use_hw);
   bool hw_decode_enabled_ = false;

 修改 gst_receiver/src/gst_receiver_node.cpp

 构造函数：删除原 gst_parse_launch 调用，替换为：
 gst_init(nullptr, nullptr);
 if (!try_build_pipeline(true)) {
   RCLCPP_WARN(get_logger(), "nvh264dec 不可用，降级到软件解码");
   if (!try_build_pipeline(false)) {
     RCLCPP_FATAL(get_logger(), "所有解码管道均失败");
     return;
   }
 }
 RCLCPP_INFO(get_logger(), "使用%s解码", hw_decode_enabled_ ? "硬件" : "软件");
 // 原有 appsink 信号绑定和 bus_watch 代码保持不变...

 新增 try_build_pipeline 方法：
 bool GstReceiverNode::try_build_pipeline(bool use_hw) {
   std::string pipeline_str;
   if (use_hw) {
     pipeline_str =
       "udpsrc port=5600 caps=\"application/x-rtp, media=video, "
       "encoding-name=H264, payload=96\" ! "
       "rtpjitterbuffer latency=50 ! rtph264depay ! h264parse ! "
       "nvh264dec ! cudadownload ! "
       "videoconvert ! video/x-raw,format=BGR ! "
       "appsink name=sink emit-signals=true sync=false max-buffers=2 drop=true";
   } else {
     pipeline_str =
       "udpsrc port=5600 caps=\"application/x-rtp, media=video, "
       "encoding-name=H264, payload=96\" ! "
       "rtpjitterbuffer latency=50 ! rtph264depay ! "
       "avdec_h264 max-threads=0 ! "
       "videoconvert ! video/x-raw,format=BGR ! "
       "appsink name=sink emit-signals=true sync=false max-buffers=2 drop=true";
   }
   GError *error = nullptr;
   GstElement *p = gst_parse_launch(pipeline_str.c_str(), &error);
   if (error || !p) {
     if (error) g_error_free(error);
     if (p) gst_object_unref(p);
     return false;
   }
   GstStateChangeReturn ret = gst_element_set_state(p, GST_STATE_PAUSED);
   if (ret == GST_STATE_CHANGE_FAILURE) {
     gst_element_set_state(p, GST_STATE_NULL);
     gst_object_unref(p);
     return false;
   }
   pipeline_ = p;
   hw_decode_enabled_ = use_hw;
   return true;
 }

 简化 on_new_sample：管道已固定输出 BGR，删除 I420 条件分支：
 // 删除 format_str 获取和 if-else 判断
 // 直接用：
 frame = cv::Mat(height, width, CV_8UC3, map.data);
 image_msg.data.assign(frame.data, frame.data + height * width * 3);

 ---
 方向3：detection_node 完整实现（P2）

 前置条件

 下载 ONNX Runtime GPU SDK 并解压到 /home/peter/onnxruntime-gpu/：
 - 结构：include/（头文件）、lib/libonnxruntime.so

 修改 detection_node/CMakeLists.txt

 在 find_package(OpenCV REQUIRED) 后添加：
 set(ONNXRUNTIME_ROOT "/home/peter/onnxruntime-gpu" CACHE PATH "ONNX Runtime root")

 修改 yolo_infer 库：
 target_include_directories(yolo_infer PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
   $<INSTALL_INTERFACE:include>
   ${ONNXRUNTIME_ROOT}/include)
 target_link_libraries(yolo_infer
   ${OpenCV_LIBS}
   ${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so)

 修改 detection_node_exe：
 ament_target_dependencies(detection_node_exe
   rclcpp sensor_msgs OpenCV robot_interfaces)

 修改 detection_node/include/detection_node/yolo_infer.hpp

 完全重写，新增：
 - Detection 和 LetterboxParams 结构体
 - ONNX Runtime 成员（Ort::Env, Ort::Session）
 - IsCudaEnabled() 方法
 - intra_op_threads、inter_op_threads 构造参数

 关键注意：Ort::Env 必须在 Ort::Session 之前声明（析构顺序）。

 修改 detection_node/src/yolo_infer.cpp

 构造函数完整实现：
 - SetIntraOpNumThreads, SetGraphOptimizationLevel(ORT_ENABLE_ALL)
 - 尝试 AppendExecutionProvider_CUDA，失败则 catch + 重置 options，降级 CPU
 - 用 GetInputNameAllocated 获取输入输出名称（存入 storage_ 避免悬空指针）

 Letterbox 修正：需记录 LetterboxParams（scale、pad_x、pad_y），供后处理反投影用

 Infer 完整实现：
 Letterbox → cvtColor(BGR→RGB) → convertTo(float32, /255) → cv::split → CHW拼接 →
 CreateTensor → session_->Run → 获取输出 [1,84,8400] → 转置为 [8400,84] → PostProcess

 PostProcess 修正：
 - YOLOv8 ONNX 输出格式：[cx,cy,w,h, score_cls0,...,score_cls79]，没有单独 objectness
 - 最大类别得分即为 confidence
 - 反 letterbox 坐标：x1 = (cx - bw/2 - pad_x) / scale
 - 调用 NMS

 注意：SimpleDetection.msg 无 header 字段，DetectionNode 发布时不要为每个 detection 设 header。

 修改 detection_node/src/detection_node.cpp

 架构：生产者-消费者模式
 - ImageCallback（ROS 回调线程）：将帧放入 frame_queue_（超过 queue_max_size_ 时丢弃最旧帧）
 - InferenceWorker（独立线程）：阻塞等待队列、取帧、调用 infer_->Infer()、发布 /detections

 关键代码：
 // 新增参数
 declare_parameter<int>("infer_queue_size", 2);
 declare_parameter<int>("ort_intra_threads", 2);

 // 推理线程内零拷贝包装（intra-process 下 msg 生命周期覆盖 Infer 全程）
 cv::Mat img(msg->height, msg->width, CV_8UC3, msg->data.data());
 auto dets = infer_->Infer(img);

 // 析构函数
 stop_flag_ = true;
 queue_cv_.notify_all();
 infer_thread_.join();

 注意：det_pub_->publish() 在非 executor 线程调用是线程安全的（rclcpp 保证）。

 修改 detection_node/config/detection.yaml

 /**:
   ros__parameters:
     detection_node:
       model_path: "models/yolov8n.onnx"
       conf_threshold: 0.25
       nms_threshold: 0.45
       use_cuda: true
       infer_queue_size: 2
       ort_intra_threads: 2

 ---
 方向2：intra-process 零拷贝（P3，依赖 P2）

 目标

 将 gst_receiver + stereo_splitter + detection_node 放入单进程，用 ROS2 intra-process 传递，
 消除 3 次跨节点深拷贝（仅保留 stereo_splitter 内部一次 460800B 的逐行拷贝，不可避免）。

 intra-process 生效条件

 - 两端节点在同一进程中
 - Publisher 调用 publish(std::unique_ptr<T>&&) 重载
 - Subscription callback 接受 std::unique_ptr<T> 参数
 - NodeOptions 启用 use_intra_process_comms(true)

 修改 gst_receiver/include/gst_receiver_node.hpp

 构造函数改为接受 NodeOptions：
 explicit GstReceiverNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());

 修改 gst_receiver/src/gst_receiver_node.cpp（on_new_sample）

 auto image_msg = std::make_unique<sensor_msgs::msg::Image>();
 // ... 填充 header、width、height、encoding、step ...
 image_msg->data.assign(frame.data, frame.data + h * w * 3);
 node->image_pub_->publish(std::move(image_msg));  // 零拷贝传递

 修改 stereo_splitter/include/stereo_splitter_node.hpp

 explicit StereoSplitterNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
 void image_callback(sensor_msgs::msg::Image::UniquePtr msg);  // UniquePtr 回调

 修改 stereo_splitter/src/stereo_splitter_node.cpp

 消除 clone() + 第二次 assign()，改为逐行 memcpy（解决 step 不连续问题）：
 void StereoSplitterNode::image_callback(sensor_msgs::msg::Image::UniquePtr msg) {
   cv::Mat full(msg->height, msg->width, CV_8UC3, msg->data.data()); // 零拷贝包装
   cv::Mat roi = full(cv::Rect(0, 0, 320, 480));  // 零拷贝 ROI view

   auto left_msg = std::make_unique<sensor_msgs::msg::Image>();
   left_msg->header = msg->header;
   left_msg->height = 480; left_msg->width = 320;
   left_msg->encoding = "bgr8"; left_msg->step = 320 * 3;
   left_msg->data.resize(480 * 320 * 3);

   for (int row = 0; row < 480; ++row)  // 逐行拷贝（原数据非连续）
     std::memcpy(left_msg->data.data() + row * 320 * 3,
                 roi.data + row * roi.step, 320 * 3);

   left_pub_->publish(std::move(left_msg));  // 零拷贝传递
 }

 修改 detection_node/src/detection_node.cpp

 构造函数接受 NodeOptions；ImageCallback 改为接受 UniquePtr。

 新建 robot_bringup/src/vision_front_main.cpp

 int main(int argc, char* argv[]) {
   rclcpp::init(argc, argv);
   rclcpp::NodeOptions opts;
   opts.use_intra_process_comms(true);
   auto executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
   executor->add_node(std::make_shared<GstReceiverNode>(opts));
   executor->add_node(std::make_shared<StereoSplitterNode>(opts));
   executor->add_node(std::make_shared<DetectionNode>(opts));
   executor->spin();
   rclcpp::shutdown();
   return 0;
 }

 修改 robot_bringup/launch/vision_stack.launch.py

 替换 gst_receiver + stereo_splitter + detection 三个独立 launch 为 vision_front 单进程节点。

 ---
 拷贝次数对比

 ┌─────────────────────────┬──────────────────────────────────┬──────────────────┐
 │          链路           │              优化前              │   优化后（P3）   │
 ├─────────────────────────┼──────────────────────────────────┼──────────────────┤
 │ gst→splitter 传递       │ 深拷贝 921600B                   │ intra 零拷贝     │
 ├─────────────────────────┼──────────────────────────────────┼──────────────────┤
 │ splitter 内部           │ clone(921600B) + assign(460800B) │ 逐行拷贝 460800B │
 ├─────────────────────────┼──────────────────────────────────┼──────────────────┤
 │ splitter→detection 传递 │ 深拷贝 460800B                   │ intra 零拷贝     │
 └─────────────────────────┴──────────────────────────────────┴──────────────────┘

 ---
 关键文件清单

 ┌──────────────────────────────────────────────────────┬──────┬────────────────────────────────────────┐
 │                         文件                         │ 方向 │                  操作                  │
 ├──────────────────────────────────────────────────────┼──────┼────────────────────────────────────────┤
 │ gst_receiver/include/gst_receiver_node.hpp           │ 1, 2 │ 新增成员/NodeOptions                   │
 ├──────────────────────────────────────────────────────┼──────┼────────────────────────────────────────┤
 │ gst_receiver/src/gst_receiver_node.cpp               │ 1, 2 │ try_build_pipeline + UniquePtr publish │
 ├──────────────────────────────────────────────────────┼──────┼────────────────────────────────────────┤
 │ detection_node/include/detection_node/yolo_infer.hpp │ 3    │ 重写，ORT 成员                         │
 ├──────────────────────────────────────────────────────┼──────┼────────────────────────────────────────┤
 │ detection_node/src/yolo_infer.cpp                    │ 3    │ 完整 ONNX 实现                         │
 ├──────────────────────────────────────────────────────┼──────┼────────────────────────────────────────┤
 │ detection_node/src/detection_node.cpp                │ 2, 3 │ UniquePtr + 推理线程                   │
 ├──────────────────────────────────────────────────────┼──────┼────────────────────────────────────────┤
 │ detection_node/CMakeLists.txt                        │ 3    │ 添加 ONNX Runtime 链接                 │
 ├──────────────────────────────────────────────────────┼──────┼────────────────────────────────────────┤
 │ detection_node/config/detection.yaml                 │ 3    │ 新增参数                               │
 ├──────────────────────────────────────────────────────┼──────┼────────────────────────────────────────┤
 │ stereo_splitter/include/stereo_splitter_node.hpp     │ 2    │ NodeOptions + UniquePtr 回调           │
 ├──────────────────────────────────────────────────────┼──────┼────────────────────────────────────────┤
 │ stereo_splitter/src/stereo_splitter_node.cpp         │ 2    │ 消除 clone + UniquePtr publish         │
 ├──────────────────────────────────────────────────────┼──────┼────────────────────────────────────────┤
 │ robot_bringup/src/vision_front_main.cpp              │ 2    │ 新建单进程入口                         │
 ├──────────────────────────────────────────────────────┼──────┼────────────────────────────────────────┤
 │ robot_bringup/launch/vision_stack.launch.py          │ 2    │ 替换为单进程启动                       │
 └──────────────────────────────────────────────────────┴──────┴────────────────────────────────────────┘

 ---
 验证方法

 # P1 验证：确认解码器选择
 ros2 run gst_receiver gst_receiver_node 2>&1 | grep -E "硬件|软件|nvh264"
 # 监测 NVDEC 使用（RTX 4060 支持 NVDEC）
 nvidia-smi dmon -s u

 # P2 验证：编译检查
 colcon build --packages-select detection_node
 # 运行并检查 CUDA 是否实际启用（从日志 YoloInfer::IsCudaEnabled）
 ros2 run detection_node detection_node_exe --ros-args --log-level debug 2>&1 | grep "推理耗时\|CUDA"

 # P3 验证：测量端到端延迟
 ros2 topic delay /camera/image_mono
 ros2 topic delay /detections