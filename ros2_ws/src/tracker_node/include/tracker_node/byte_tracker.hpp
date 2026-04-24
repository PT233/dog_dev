#pragma once

#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

namespace tracker_node {

struct Detection {
  float x, y, w, h;  // bbox center and size
  float conf;
  int class_id;

  float Area() const { return w * h; }
  float CenterDistance(const Detection& other) const {
    float dx = x - other.x;
    float dy = y - other.y;
    return std::sqrt(dx * dx + dy * dy);
  }
};

struct TrackState {
  int track_id;
  Detection last_detection;
  int age = 0;
  int active_frames = 0;
  int miss_frames = 0;

  bool IsActive() const { return active_frames >= 1; }
  bool IsToDelete(int max_buffer) const { return miss_frames > max_buffer; }
};

class ByteTracker {
public:
  ByteTracker(int track_buffer = 30, float track_thresh = 0.5f, float match_thresh = 0.5f);

  std::vector<std::pair<int, Detection>> Update(const std::vector<Detection>& detections);

  void Reset();

private:
  int track_buffer_;
  float track_thresh_;
  float match_thresh_;
  int next_id_ = 1;

  std::map<int, TrackState> active_tracks_;

  float ComputeIoU(const Detection& det, const Detection& track_det) const;
  float ComputeCost(const Detection& det, const Detection& track_det) const;
};

}  // namespace tracker_node
