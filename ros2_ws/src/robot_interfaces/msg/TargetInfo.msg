# 当前目标选择结果，预留给更完整的行为/状态显示。
std_msgs/Header header

# 目标类别名和跟踪 ID。
string class_name
string track_id

# 当前目标置信度和检测框像素位置。
float32 confidence
int32 bbox_cx
int32 bbox_cy
int32 bbox_w
int32 bbox_h

# true 表示当前行为节点已经锁定一个有效目标。
bool locked
