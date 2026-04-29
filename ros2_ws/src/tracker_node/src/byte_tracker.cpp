#include "tracker_node/byte_tracker.hpp"

namespace tracker_node
{

ByteTracker::ByteTracker(int track_buffer, float confidence_threshold, float match_threshold)
: track_buffer_(track_buffer),
  confidence_threshold_(confidence_threshold),
  match_threshold_(match_threshold) {}

std::vector<std::pair<int, Detection>> ByteTracker::update(
  const std::vector<Detection> & detections)
{
  std::vector<std::pair<int, Detection>> result;

  // 阶段 1：过滤低置信度检测（低于 confidence_threshold_ 的检测不参与匹配）
  std::vector<Detection> high_confidence_detections;
  for (const auto & detection : detections) {
    if (detection.confidence >= confidence_threshold_) {
      high_confidence_detections.push_back(detection);
    }
  }

  // 阶段 2：获取当前所有活跃轨迹的快照，用于本帧匹配
  std::vector<TrackState> tracks;
  std::vector<int> track_ids;
  for (auto & [id, state] : active_tracks_) {
    tracks.push_back(state);
    track_ids.push_back(id);
  }

  // 阶段 3：贪心匹配（对每个检测框，找代价最小的未匹配轨迹）
  // 注：这是简化版，正式 ByteTrack 使用匈牙利算法；此处在目标数量少（<10）时效果相当
  std::vector<bool> matched_detection(high_confidence_detections.size(), false);
  std::vector<bool> matched_track(tracks.size(), false);

  for (size_t detection_index = 0; detection_index < high_confidence_detections.size();
    ++detection_index)
  {
    int best_track_index = -1;
    float best_cost = match_threshold_;  // 代价超过阈值则视为未匹配（1-IoU > match_threshold）
    for (size_t j = 0; j < tracks.size(); ++j) {
      if (!matched_track[j]) {
        float cost =
          compute_cost(high_confidence_detections[detection_index], tracks[j].last_detection);
        if (cost < best_cost) {
          best_cost = cost;
          best_track_index = static_cast<int>(j);
        }
      }
    }

    if (best_track_index >= 0) {
      int track_id = track_ids[best_track_index];
      active_tracks_[track_id].last_detection = high_confidence_detections[detection_index];
      active_tracks_[track_id].miss_frames = 0;
      active_tracks_[track_id].active_frames++;
      result.push_back({track_id, high_confidence_detections[detection_index]});
      matched_detection[detection_index] = true;
      matched_track[best_track_index] = true;
    }
  }

  // 阶段 4：未匹配的检测 → 创建新轨迹（分配新 ID）
  for (size_t detection_index = 0; detection_index < high_confidence_detections.size();
    ++detection_index)
  {
    if (!matched_detection[detection_index]) {
      int new_id = next_id_++;
      TrackState state;
      state.track_id = new_id;
      state.last_detection = high_confidence_detections[detection_index];
      state.age = 0;
      state.active_frames = 1;
      state.miss_frames = 0;
      active_tracks_[new_id] = state;
      result.push_back({new_id, high_confidence_detections[detection_index]});
    }
  }

  // 阶段 5：未匹配的轨迹 → miss_frames++，超过 track_buffer_ 则删除
  // track_buffer_ 默认 30 帧（约 1 秒），允许目标短暂遮挡后恢复同一 ID
  auto it = active_tracks_.begin();
  while (it != active_tracks_.end()) {
    auto & [id, state] = *it;
    bool matched = false;
    for (size_t j = 0; j < tracks.size(); ++j) {
      if (matched_track[j] && track_ids[j] == id) {
        matched = true;
        break;
      }
    }

    if (!matched) {
      state.miss_frames++;
      if (state.should_delete(track_buffer_)) {
        it = active_tracks_.erase(it);
      } else {
        ++it;
      }
    } else {
      ++it;
    }
  }

  return result;
}

float ByteTracker::compute_iou(const Detection & detection1, const Detection & detection2) const
{
  // 输入检测框是中心点格式，先转换成左上/右下角坐标再计算交并比。
  float detection1_min_x = detection1.center_x - detection1.width / 2;
  float detection1_min_y = detection1.center_y - detection1.height / 2;
  float detection1_max_x = detection1.center_x + detection1.width / 2;
  float detection1_max_y = detection1.center_y + detection1.height / 2;

  float detection2_min_x = detection2.center_x - detection2.width / 2;
  float detection2_min_y = detection2.center_y - detection2.height / 2;
  float detection2_max_x = detection2.center_x + detection2.width / 2;
  float detection2_max_y = detection2.center_y + detection2.height / 2;

  float intersection_min_x = std::max(detection1_min_x, detection2_min_x);
  float intersection_min_y = std::max(detection1_min_y, detection2_min_y);
  float intersection_max_x = std::min(detection1_max_x, detection2_max_x);
  float intersection_max_y = std::min(detection1_max_y, detection2_max_y);

  if (intersection_max_x < intersection_min_x || intersection_max_y < intersection_min_y) {
    return 0.0f;
  }

  float intersection_area =
    (intersection_max_x - intersection_min_x) * (intersection_max_y - intersection_min_y);
  float union_area = detection1.area() + detection2.area() - intersection_area;

  return intersection_area / (union_area + 1e-6f);
}

float ByteTracker::compute_cost(
  const Detection & detection, const Detection & track_detection) const
{
  float iou = compute_iou(detection, track_detection);
  return 1.0f - iou;  // 代价越小代表越可能是同一个目标。
}

void ByteTracker::reset()
{
  active_tracks_.clear();
  next_id_ = 1;
}

}  // namespace tracker_node
