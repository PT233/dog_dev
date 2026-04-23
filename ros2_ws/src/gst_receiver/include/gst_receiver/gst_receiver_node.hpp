#pragma once

#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"

class GstReceiverNode : public rclcpp::Node {
public:
  GstReceiverNode();
  ~GstReceiverNode();

private:
  GstElement *pipeline_;
  GstBus *bus_;

  static gboolean on_bus_message(GstBus *bus, GstMessage *msg, gpointer user_data);
  static void on_new_sample(GstElement *appsink, gpointer user_data);
};
