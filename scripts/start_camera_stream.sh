#!/bin/bash

# Raspberry Pi camera stream start script
# Reads from USB stereo camera, encodes via H.264 hardware encoder, streams over UDP to WSL2

TARGET_IP=${1:-192.168.137.1}
TARGET_PORT=${2:-5600}

echo "Starting camera stream to $TARGET_IP:$TARGET_PORT..."

gst-launch-1.0 -v \
  v4l2src device=/dev/video0 ! \
  'video/x-raw,format=YUY2,width=640,height=480,framerate=30/1' ! \
  videoconvert ! \
  v4l2h264enc ! 'video/x-h264,level=(string)4' ! h264parse ! \
  rtph264pay config-interval=1 ! \
  udpsink host=$TARGET_IP port=$TARGET_PORT
