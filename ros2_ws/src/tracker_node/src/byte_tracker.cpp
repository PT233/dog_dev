#include "tracker_node/byte_tracker.hpp"

namespace tracker_node {

ByteTracker::ByteTracker(int track_buffer, float track_thresh, float match_thresh)
    : track_buffer_(track_buffer), track_thresh_(track_thresh), match_thresh_(match_thresh) {}

std::vector<std::pair<int, Detection>> ByteTracker::Update(
    const std::vector<Detection>& detections) {
  std::vector<std::pair<int, Detection>> result;

  // Filter high-confidence detections
  std::vector<Detection> high_conf;
  for (const auto& det : detections) {
    if (det.conf >= track_thresh_) {
      high_conf.push_back(det);
    }
  }

  // Get tracks for matching
  std::vector<TrackState> tracks;
  std::vector<int> track_ids;
  for (auto& [id, state] : active_tracks_) {
    tracks.push_back(state);
    track_ids.push_back(id);
  }

  // Match with active tracks using greedy assignment
  std::vector<bool> matched_det(high_conf.size(), false);
  std::vector<bool> matched_track(tracks.size(), false);

  for (size_t i = 0; i < high_conf.size(); ++i) {
    int best_j = -1;
    float best_cost = match_thresh_;
    for (size_t j = 0; j < tracks.size(); ++j) {
      if (!matched_track[j]) {
        float cost = ComputeCost(high_conf[i], tracks[j].last_detection);
        if (cost < best_cost) {
          best_cost = cost;
          best_j = j;
        }
      }
    }

    if (best_j >= 0) {
      int track_id = track_ids[best_j];
      active_tracks_[track_id].last_detection = high_conf[i];
      active_tracks_[track_id].miss_frames = 0;
      active_tracks_[track_id].active_frames++;
      result.push_back({track_id, high_conf[i]});
      matched_det[i] = true;
      matched_track[best_j] = true;
    }
  }

  // Create new tracks from unmatched detections
  for (size_t i = 0; i < high_conf.size(); ++i) {
    if (!matched_det[i]) {
      int new_id = next_id_++;
      TrackState state;
      state.track_id = new_id;
      state.last_detection = high_conf[i];
      state.age = 0;
      state.active_frames = 1;
      state.miss_frames = 0;
      active_tracks_[new_id] = state;
      result.push_back({new_id, high_conf[i]});
    }
  }

  // Update unmatched tracks and remove dead ones
  auto it = active_tracks_.begin();
  while (it != active_tracks_.end()) {
    auto& [id, state] = *it;
    bool matched = false;
    for (size_t j = 0; j < tracks.size(); ++j) {
      if (matched_track[j] && track_ids[j] == id) {
        matched = true;
        break;
      }
    }

    if (!matched) {
      state.miss_frames++;
      if (state.IsToDelete(track_buffer_)) {
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

float ByteTracker::ComputeIoU(const Detection& det1, const Detection& det2) const {
  float x1_min = det1.x - det1.w / 2;
  float y1_min = det1.y - det1.h / 2;
  float x1_max = det1.x + det1.w / 2;
  float y1_max = det1.y + det1.h / 2;

  float x2_min = det2.x - det2.w / 2;
  float y2_min = det2.y - det2.h / 2;
  float x2_max = det2.x + det2.w / 2;
  float y2_max = det2.y + det2.h / 2;

  float inter_x_min = std::max(x1_min, x2_min);
  float inter_y_min = std::max(y1_min, y2_min);
  float inter_x_max = std::min(x1_max, x2_max);
  float inter_y_max = std::min(y1_max, y2_max);

  if (inter_x_max < inter_x_min || inter_y_max < inter_y_min) {
    return 0.0f;
  }

  float inter_area = (inter_x_max - inter_x_min) * (inter_y_max - inter_y_min);
  float union_area = det1.Area() + det2.Area() - inter_area;

  return inter_area / (union_area + 1e-6f);
}

float ByteTracker::ComputeCost(const Detection& det, const Detection& track_det) const {
  float iou = ComputeIoU(det, track_det);
  return 1.0f - iou;  // 1 - IoU as cost
}

void ByteTracker::Reset() {
  active_tracks_.clear();
  next_id_ = 1;
}

}  // namespace tracker_node
