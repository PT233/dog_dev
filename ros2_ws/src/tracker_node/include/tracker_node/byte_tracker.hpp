#pragma once

// 简化版 ByteTrack 目标跟踪算法
// 以 1-IoU 为代价进行贪心匹配（非匈牙利算法），
// 维护活跃轨迹列表（map<id, TrackState>），处理目标的出现、遮挡和消失。

#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

namespace tracker_node {

// 单帧检测结果（中心坐标格式，与 SimpleDetection.msg 对应）
struct Detection {
  float x, y, w, h;  // 检测框中心 (x,y) 和尺寸 (w,h)，像素单位
  float conf;         // 置信度（objectness × max_class_prob）
  int class_id;       // COCO 类别 ID

  float Area() const { return w * h; }
  float CenterDistance(const Detection& other) const {
    float dx = x - other.x;
    float dy = y - other.y;
    return std::sqrt(dx * dx + dy * dy);
  }
};

// 单条轨迹的状态（在 ByteTracker::active_tracks_ 中维护）
struct TrackState {
  int track_id;
  Detection last_detection;  // 最后一次匹配到的检测框（用于下一帧 IoU 计算）
  int age = 0;               // 轨迹创建以来的总帧数
  int active_frames = 0;     // 持续匹配到检测的帧数
  int miss_frames = 0;       // 连续未匹配到检测的帧数

  bool IsActive() const { return active_frames >= 1; }
  // miss_frames 超过 track_buffer 时返回 true，触发轨迹删除
  bool IsToDelete(int max_buffer) const { return miss_frames > max_buffer; }
};

// ByteTrack 跟踪器（简化版，贪心 IoU 匹配）
// Update() 每帧调用一次，返回 (track_id, Detection) 对列表
class ByteTracker {
public:
  // track_buffer: 目标消失后保留的最大帧数（默认 30 帧 ≈ 1 秒@30fps）
  // track_thresh: 检测置信度阈值，低于此值不参与匹配
  // match_thresh: 匹配代价阈值（1-IoU），超过此值视为未匹配
  ByteTracker(int track_buffer = 30, float track_thresh = 0.5f, float match_thresh = 0.5f);

  // 输入本帧检测列表，返回已关联 track_id 的 (id, detection) 对
  std::vector<std::pair<int, Detection>> Update(const std::vector<Detection>& detections);

  // 清空所有轨迹，next_id_ 重置为 1
  void Reset();

private:
  int track_buffer_;    // 轨迹保留帧数
  float track_thresh_;  // 最低置信度阈值
  float match_thresh_;  // 最大匹配代价（1-IoU）
  int next_id_ = 1;     // 下一个轨迹 ID（全局单调递增）

  std::map<int, TrackState> active_tracks_;  // 当前活跃轨迹（id → 状态）

  // 计算两个检测框的 IoU（Intersection over Union）
  float ComputeIoU(const Detection& det, const Detection& track_det) const;
  // 匹配代价 = 1 - IoU（值越小越好）
  float ComputeCost(const Detection& det, const Detection& track_det) const;
};

}  // namespace tracker_node
