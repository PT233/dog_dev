#include "gst_receiver/gst_receiver_node.hpp"
#include <gst/app/gstappsink.h>
#include <gst/gstutils.h>

GstReceiverNode::GstReceiverNode(const rclcpp::NodeOptions& options)
    : Node("gst_receiver_node", options), pipeline_(nullptr), bus_(nullptr) {
  RCLCPP_INFO(this->get_logger(), "gst_receiver_node started");

  // Create image publisher
  image_pub_ = this->create_publisher<sensor_msgs::msg::Image>("/stereo/image_raw", rclcpp::SensorDataQoS());

  // Initialize GStreamer
  gst_init(nullptr, nullptr);

  // Try hardware decoding (nvh264dec), fall back to software if failed
  if (!try_build_pipeline(true)) {
    RCLCPP_WARN(this->get_logger(), "nvh264dec unavailable, falling back to software decoding");
    if (!try_build_pipeline(false)) {
      RCLCPP_FATAL(this->get_logger(), "Both hardware and software decoding pipelines failed");
      return;
    }
  }

  RCLCPP_INFO(this->get_logger(), "Using %s decoding (H.264)", hw_decode_enabled_ ? "hardware" : "software");

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
  RCLCPP_INFO(this->get_logger(), "GStreamer pipeline started, publishing to /stereo/image_raw");
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

bool GstReceiverNode::try_build_pipeline(bool use_hw) {
  std::string pipeline_str;
  if (use_hw) {
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
    if (p) gst_object_unref(p);
    return false;
  }

  GstStateChangeReturn ret = gst_element_set_state(p, GST_STATE_PAUSED);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    RCLCPP_DEBUG(this->get_logger(), "Failed to set pipeline to PAUSED state");
    gst_element_set_state(p, GST_STATE_NULL);
    gst_object_unref(p);
    return false;
  }

  pipeline_ = p;
  hw_decode_enabled_ = use_hw;
  return true;
}

gboolean GstReceiverNode::on_bus_message(GstBus * /* bus */, GstMessage *msg, gpointer user_data) {
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
    GstCaps *caps = gst_sample_get_caps(sample);
    GstStructure *structure = gst_caps_get_structure(caps, 0);

    gint width, height;
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);

    // Map buffer and create cv::Mat (pipeline output is fixed to BGR)
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
      // Pipeline output is fixed to BGR format via videoconvert
      cv::Mat frame = cv::Mat(height, width, CV_8UC3, map.data);

      // Convert to ROS Image message
      auto image_msg = std::make_unique<sensor_msgs::msg::Image>();
      image_msg->header.stamp = node->now();
      image_msg->header.frame_id = "camera";
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

