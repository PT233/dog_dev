#include "gst_receiver/gst_receiver_node.hpp"
#include <gst/app/gstappsink.h>
#include <gst/gstutils.h>

GstReceiverNode::GstReceiverNode() : Node("gst_receiver_node"), pipeline_(nullptr), bus_(nullptr) {
  RCLCPP_INFO(this->get_logger(), "gst_receiver_node started");

  // Create image publisher
  image_pub_ = this->create_publisher<sensor_msgs::msg::Image>("/stereo/image_raw", rclcpp::SensorDataQoS());

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
    const gchar *format_str;
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);
    format_str = gst_structure_get_string(structure, "format");

    // Map buffer and create cv::Mat
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
      cv::Mat frame;

      // Create frame based on format
      if (format_str && std::string(format_str) == "I420") {
        // I420 format (YUV planar)
        frame = cv::Mat(height + height / 2, width, CV_8UC1, map.data);
        cv::Mat bgr_frame;
        cv::cvtColor(frame, bgr_frame, cv::COLOR_YUV2BGR_I420);
        frame = bgr_frame;
      } else {
        // Default: treat as BGR
        frame = cv::Mat(height, width, CV_8UC3, map.data);
      }

      // Convert to ROS Image message
      std_msgs::msg::Header header;
      header.stamp = node->now();
      header.frame_id = "camera";

      sensor_msgs::msg::Image image_msg;
      image_msg.header = header;
      image_msg.height = height;
      image_msg.width = width;
      image_msg.encoding = "bgr8";
      image_msg.is_bigendian = false;
      image_msg.step = width * 3;
      image_msg.data.assign(frame.data, frame.data + (height * width * 3));

      node->image_pub_->publish(image_msg);

      RCLCPP_DEBUG(node->get_logger(), "Published frame: %dx%d (%s)", width, height, format_str ? format_str : "unknown");

      gst_buffer_unmap(buffer, &map);
    }

    gst_sample_unref(sample);
  }
}

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<GstReceiverNode>());
  rclcpp::shutdown();
  return 0;
}
