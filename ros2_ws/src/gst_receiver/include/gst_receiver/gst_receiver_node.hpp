#pragma once

#include <gst/gst.h>
#include <opencv2/opencv.hpp>
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "std_msgs/msg/header.hpp"

// GStreamer UDP/H.264 接收节点
//
// 从树莓派摄像头推流端接收 RTP/H.264，解码为 BGR 图像后发布 /stereo/image_raw。
// 优先尝试 NVIDIA 硬解码，失败后自动降级为软件解码。
class GstReceiverNode : public rclcpp::Node {
public:
  explicit GstReceiverNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
  ~GstReceiverNode();

private:
  GstElement *pipeline_;  // 当前 GStreamer pipeline
  GstBus *bus_;           // pipeline 消息总线，用于接收错误和 EOS
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;
  bool hw_decode_enabled_ = false;  // true 表示当前使用 nvh264dec

  // 构建硬解或软解 pipeline，成功后保存到 pipeline_。
  bool try_build_pipeline(bool use_hw);

  // GStreamer bus 回调：打印 pipeline 错误和流结束事件。
  static gboolean on_bus_message(GstBus *bus, GstMessage *msg, gpointer user_data);

  // appsink 回调：把解码帧转换为 ROS Image 消息。
  static void on_new_sample(GstElement *appsink, gpointer user_data);
};
