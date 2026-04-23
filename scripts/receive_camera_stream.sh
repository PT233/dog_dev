#!/bin/bash

# WSL2 camera stream receiver script
# Receives H.264 RTP stream from Raspberry Pi and displays in real-time

LISTEN_PORT=${1:-5600}

echo "Listening for camera stream on UDP port $LISTEN_PORT..."
echo "Make sure Raspberry Pi is running: ssh ubuntu@192.168.137.100 '~/start_camera_stream.sh'"
echo ""

gst-launch-1.0 -v \
  udpsrc port=$LISTEN_PORT \
    caps="application/x-rtp, media=video, encoding-name=H264, payload=96" ! \
  rtpjitterbuffer ! \
  rtph264depay ! \
  avdec_h264 ! \
  videoconvert ! \
  autovideosink sync=false
