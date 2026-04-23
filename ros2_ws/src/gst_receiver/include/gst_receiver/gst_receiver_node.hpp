#pragma once

#include <gst/gst.h>
#include <opencv2/opencv.hpp>
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "std_msgs/msg/header.hpp"

class GstReceiverNode : public rclcpp::Node {
public:
  GstReceiverNode();
  ~GstReceiverNode();

private:
  GstElement *pipeline_;
  GstBus *bus_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_pub_;

  static gboolean on_bus_message(GstBus *bus, GstMessage *msg, gpointer user_data);
  static void on_new_sample(GstElement *appsink, gpointer user_data);
};
