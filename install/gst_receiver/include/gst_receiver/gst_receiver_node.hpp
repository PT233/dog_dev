#pragma once

#include <gst/gst.h>
#include <opencv2/opencv.hpp>
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "std_msgs/msg/header.hpp"

class GstReceiverNode : public rclcpp::Node {
public:
  explicit GstReceiverNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
  ~GstReceiverNode();

private:
  GstElement *pipeline_;
  GstBus *bus_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;
  bool hw_decode_enabled_ = false;

  bool try_build_pipeline(bool use_hw);
  static gboolean on_bus_message(GstBus *bus, GstMessage *msg, gpointer user_data);
  static void on_new_sample(GstElement *appsink, gpointer user_data);
};
