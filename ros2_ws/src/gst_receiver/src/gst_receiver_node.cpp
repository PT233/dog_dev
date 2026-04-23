#include "gst_receiver/gst_receiver_node.hpp"
#include <gst/app/gstappsink.h>
#include <gst/gstutils.h>

GstReceiverNode::GstReceiverNode() : Node("gst_receiver_node"), pipeline_(nullptr), bus_(nullptr) {
  RCLCPP_INFO(this->get_logger(), "gst_receiver_node started");

  // Initialize GStreamer
  gst_init(nullptr, nullptr);

  // Create pipeline string
  const char *pipeline_str =
    "udpsrc port=5600 caps=\"application/x-rtp, media=video, encoding-name=H264, payload=96\" ! "
    "rtpjitterbuffer ! "
    "rtph264depay ! "
    "avdec_h264 ! "
    "videoconvert ! "
    "appsink name=sink emit-signals=true";

  GError *error = nullptr;
  pipeline_ = gst_parse_launch(pipeline_str, &error);

  if (error) {
    RCLCPP_ERROR(this->get_logger(), "Failed to create pipeline: %s", error->message);
    g_error_free(error);
    return;
  }

  // Get appsink element
  GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline_), "sink");
  if (appsink) {
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), this);
    gst_object_unref(appsink);
  }

  // Add bus watch
  bus_ = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
  gst_bus_add_watch(bus_, on_bus_message, this);

  // Start pipeline
  gst_element_set_state(pipeline_, GST_STATE_PLAYING);
  RCLCPP_INFO(this->get_logger(), "GStreamer pipeline started");
}

GstReceiverNode::~GstReceiverNode() {
  if (pipeline_) {
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
  }
  if (bus_) {
    gst_object_unref(bus_);
  }
  gst_deinit();
}

gboolean GstReceiverNode::on_bus_message(GstBus *bus, GstMessage *msg, gpointer user_data) {
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

void GstReceiverNode::on_new_sample(GstElement *appsink, gpointer user_data) {
  GstReceiverNode *node = static_cast<GstReceiverNode *>(user_data);

  GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
  if (sample) {
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    gsize size = gst_buffer_get_size(buffer);

    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *structure = gst_caps_get_structure(caps, 0);
    gint width, height;
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);

    RCLCPP_DEBUG(node->get_logger(), "Got frame: %dx%d, size=%lu", width, height, size);

    gst_sample_unref(sample);
  }
}

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<GstReceiverNode>());
  rclcpp::shutdown();
  return 0;
}
