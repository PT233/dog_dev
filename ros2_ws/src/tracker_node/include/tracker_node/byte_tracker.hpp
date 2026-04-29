#ifndef TRACKER_NODE__BYTE_TRACKER_HPP_
#define TRACKER_NODE__BYTE_TRACKER_HPP_

// 简化版 ByteTrack 目标跟踪算法
// 以 1-IoU 为代价进行贪心匹配（非匈牙利算法），
// 维护活跃轨迹列表（map<id, TrackState>），处理目标的出现、遮挡和消失。

#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

namespace tracker_node
{

// 单帧检测结果（中心坐标格式，与 Detection2D.msg 对应）
struct Detection
{
  float center_x;
  float center_y;
  float width;
  float height;
  float confidence;  // 置信度（objectness × max_class_prob）
  int class_id;       // COCO 类别 ID

  float area() const {return width * height;}
  float center_distance(const Detection & other) const
  {
    float delta_x = center_x - other.center_x;
    float delta_y = center_y - other.center_y;
    return std::sqrt(delta_x * delta_x + delta_y * delta_y);
  }
};

// 单条轨迹的状态（在 ByteTracker::active_tracks_ 中维护）
struct TrackState
{
  int track_id;
  Detection last_detection;  // 最后一次匹配到的检测框（用于下一帧 IoU 计算）
  int age = 0;               // 轨迹创建以来的总帧数
  int active_frames = 0;     // 持续匹配到检测的帧数
  int miss_frames = 0;       // 连续未匹配到检测的帧数

  bool is_active() const {return active_frames >= 1;}
  // miss_frames 超过 track_buffer 时返回 true，触发轨迹删除
  bool should_delete(int max_buffer) const {return miss_frames > max_buffer;}
};

// ByteTrack 跟踪器（简化版，贪心 IoU 匹配）
// update() 每帧调用一次，返回 (track_id, Detection) 对列表
class ByteTracker {
public:
  // track_buffer: 目标消失后保留的最大帧数（默认 30 帧 ≈ 1 秒@30fps）
  // confidence_threshold: 检测置信度阈值，低于此值不参与匹配
  // match_threshold: 匹配代价阈值（1-IoU），超过此值视为未匹配
  ByteTracker(
    int track_buffer = 30, float confidence_threshold = 0.5f,
    float match_threshold = 0.5f);

  // 输入本帧检测列表，返回已关联 track_id 的 (id, detection) 对
  std::vector<std::pair<int, Detection>> update(const std::vector<Detection> & detections);

  // 清空所有轨迹，next_id_ 重置为 1
  void reset();

private:
  int track_buffer_;           // 轨迹保留帧数
  float confidence_threshold_;  // 最低置信度阈值
  float match_threshold_;       // 最大匹配代价（1-IoU）
  int next_id_ = 1;             // 下一个轨迹 ID（全局单调递增）

  std::map<int, TrackState> active_tracks_;  // 当前活跃轨迹（id → 状态）

  // 计算两个检测框的 IoU（Intersection over Union）
  float compute_iou(const Detection & detection, const Detection & track_detection) const;
  // 匹配代价 = 1 - IoU（值越小越好）
  float compute_cost(const Detection & detection, const Detection & track_detection) const;
};

}  // namespace tracker_node

#endif  // TRACKER_NODE__BYTE_TRACKER_HPP_
