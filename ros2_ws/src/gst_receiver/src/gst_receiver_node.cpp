#include "gst_receiver/gst_receiver_node.hpp"
#include <gst/app/gstappsink.h>
#include <gst/gstutils.h>

namespace gst_receiver
{

GstReceiverNode::GstReceiverNode(const rclcpp::NodeOptions & options)
: Node("gst_receiver_node", options), pipeline_(nullptr), bus_(nullptr)
{
  RCLCPP_INFO(this->get_logger(), "gst_receiver_node started");

  // 发布完整左右拼接图像，后续 stereo_splitter 会取左半幅。
  image_pub_ =
    this->create_publisher<sensor_msgs::msg::Image>("~/output/stereo_image_raw",
                                                      rclcpp::SensorDataQoS());

  // 初始化 GStreamer，全进程只需要一次，但重复调用是安全的。
  gst_init(nullptr, nullptr);

  // 优先硬解码以降低 CPU 占用；没有 NVIDIA 插件时自动回退软件解码。
  if (!try_build_pipeline(true)) {
    RCLCPP_WARN(this->get_logger(), "nvh264dec unavailable, falling back to software decoding");
    if (!try_build_pipeline(false)) {
      RCLCPP_FATAL(this->get_logger(), "Both hardware and software decoding pipelines failed");
      return;
    }
  }

  RCLCPP_INFO(this->get_logger(), "Using %s decoding (H.264)",
              is_hw_decode_enabled_ ? "hardware" : "software");

  // appsink 以回调方式把解码后的 BGR 帧交给 ROS 节点。
  GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");
  if (appsink) {
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), this);
    gst_object_unref(appsink);
  }

  // bus watch 负责捕获 pipeline 错误，避免解码失败时无日志。
  bus_ = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
  gst_bus_add_watch(bus_, on_bus_message, this);

  // pipeline 进入 PLAYING 后开始接收 UDP 5600 端口数据。
  gst_element_set_state(pipeline_, GST_STATE_PLAYING);
  RCLCPP_INFO(this->get_logger(),
              "GStreamer pipeline started, publishing to ~/output/stereo_image_raw");
}

GstReceiverNode::~GstReceiverNode()
{
  if (pipeline_) {
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
  }
  if (bus_) {
    gst_object_unref(bus_);
  }
  gst_deinit();
}

bool GstReceiverNode::try_build_pipeline(bool use_hw)
{
  std::string pipeline_str;
  if (use_hw) {
    // 硬解路径：RTP 抖动缓冲 -> H264 解包/解析 -> NVIDIA 解码 -> 下载到 CPU BGR。
    pipeline_str =
      "udpsrc port=5600 caps=\"application/x-rtp, media=video, "
      "encoding-name=H264, payload=96\" ! "
      "rtpjitterbuffer latency=50 ! "
      "rtph264depay ! h264parse ! "
      "nvh264dec ! "
      "cudadownload ! "
      "videoconvert ! video/x-raw,format=BGR ! "
      "appsink name=sink emit-signals=true sync=false max-buffers=2 drop=true";
  } else {
    // 软件解码路径：avdec_h264 使用 CPU 解码，兼容无 NVIDIA GPU 的开发环境。
    pipeline_str =
      "udpsrc port=5600 caps=\"application/x-rtp, media=video, "
      "encoding-name=H264, payload=96\" ! "
      "rtpjitterbuffer latency=50 ! "
      "rtph264depay ! "
      "avdec_h264 max-threads=0 ! "
      "videoconvert ! video/x-raw,format=BGR ! "
      "appsink name=sink emit-signals=true sync=false max-buffers=2 drop=true";
  }

  GError *error = nullptr;
  GstElement *p = gst_parse_launch(pipeline_str.c_str(), &error);

  if (error || !p) {
    if (error) {
      if (use_hw) {
        RCLCPP_DEBUG(this->get_logger(), "Hardware pipeline parse error: %s", error->message);
      }
      g_error_free(error);
    }
    if (p) {gst_object_unref(p);}
    return false;
  }

  GstStateChangeReturn ret = gst_element_set_state(p, GST_STATE_PAUSED);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    // 能解析不代表能运行，先切到 PAUSED 验证插件和 caps 能否协商。
    RCLCPP_DEBUG(this->get_logger(), "Failed to set pipeline to PAUSED state");
    gst_element_set_state(p, GST_STATE_NULL);
    gst_object_unref(p);
    return false;
  }

  pipeline_ = p;
  is_hw_decode_enabled_ = use_hw;
  return true;
}

gboolean GstReceiverNode::on_bus_message(GstBus * /* bus */, GstMessage *msg, gpointer user_data)
{
  GstReceiverNode *node = static_cast<GstReceiverNode *>(user_data);

  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
        GError *err = nullptr;
        gchar *debug = nullptr;
        gst_message_parse_error(msg, &err, &debug);
        RCLCPP_ERROR(node->get_logger(), "GStreamer error: %s", err->message);
        g_error_free(err);
        g_free(debug);
        break;
      }
    case GST_MESSAGE_EOS:
      RCLCPP_INFO(node->get_logger(), "End of stream");
      break;
    default:
      break;
  }
  return TRUE;
}

void GstReceiverNode::on_new_sample(GstElement *appsink, gpointer user_data)
{
  GstReceiverNode *node = static_cast<GstReceiverNode *>(user_data);

  GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
  if (sample) {
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *structure = gst_caps_get_structure(caps, 0);

    gint width, height;
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);

    // 映射 buffer 并创建 cv::Mat 视图，pipeline 输出已固定为 BGR。
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
      // Mat 只引用 GStreamer buffer 数据，发布前需要复制到 ROS Image。
      cv::Mat frame = cv::Mat(height, width, CV_8UC3, map.data);

      // 转为 ROS Image；该图像仍是双目拼接帧，使用 REP-105 风格 frame_id。
      auto image_msg = std::make_unique<sensor_msgs::msg::Image>();
      image_msg->header.stamp = node->now();
      image_msg->header.frame_id = "stereo_camera_link";
      image_msg->height = height;
      image_msg->width = width;
      image_msg->encoding = "bgr8";
      image_msg->is_bigendian = false;
      image_msg->step = width * 3;
      image_msg->data.assign(frame.data, frame.data + (height * width * 3));

      node->image_pub_->publish(std::move(image_msg));

      RCLCPP_DEBUG(node->get_logger(), "Published frame: %dx%d", width, height);

      gst_buffer_unmap(buffer, &map);
    }

    gst_sample_unref(sample);
  }
}

}  // namespace gst_receiver
