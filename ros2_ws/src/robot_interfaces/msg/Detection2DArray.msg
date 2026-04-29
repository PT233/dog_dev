# 一帧检测/跟踪结果。
# header.stamp 应沿用原始图像时间戳，便于可视化节点找到对应画面。
std_msgs/Header header

# 同一帧内的所有检测框。
Detection2D[] detections
