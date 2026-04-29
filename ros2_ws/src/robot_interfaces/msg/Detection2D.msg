# 单个 2D 检测框。
# 坐标采用图像像素坐标，center_x/center_y 是检测框中心点，width/height 是框尺寸。
float32 center_x
float32 center_y
float32 width
float32 height

# YOLO 输出的最终置信度，通常为 objectness × class_probability。
float32 confidence

# COCO 类别 ID；类别名由各节点通过 models/coco_classes.txt 映射。
int32 class_id

# tracker_node 填写的轨迹 ID；检测节点原始输出时为空字符串。
string track_id
