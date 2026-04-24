#include <iostream>
#include <vector>
#include "tracker_node/byte_tracker.hpp"

using namespace tracker_node;

int main() {
  ByteTracker tracker(30, 0.5f, 0.5f);

  std::cout << "=== Test 1: Two detections on first frame ===" << std::endl;
  std::vector<Detection> frame1 = {
    {100.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0},
    {200.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0}
  };

  auto result1 = tracker.Update(frame1);
  std::cout << "Frame 1 results:" << std::endl;
  for (const auto& [id, det] : result1) {
    std::cout << "  Track ID: " << id << ", Pos: (" << det.x << ", " << det.y << ")" << std::endl;
  }

  std::cout << "\n=== Test 2: Same detections on second frame ===" << std::endl;
  std::vector<Detection> frame2 = {
    {102.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0},  // slightly shifted
    {198.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0}   // slightly shifted
  };

  auto result2 = tracker.Update(frame2);
  std::cout << "Frame 2 results:" << std::endl;
  for (const auto& [id, det] : result2) {
    std::cout << "  Track ID: " << id << ", Pos: (" << det.x << ", " << det.y << ")" << std::endl;
  }

  std::cout << "\n=== Test 3: New detection at different location ===" << std::endl;
  std::vector<Detection> frame3 = {
    {102.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0},
    {198.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0},
    {300.0f, 300.0f, 50.0f, 50.0f, 0.9f, 0}  // completely new
  };

  auto result3 = tracker.Update(frame3);
  std::cout << "Frame 3 results:" << std::endl;
  for (const auto& [id, det] : result3) {
    std::cout << "  Track ID: " << id << ", Pos: (" << det.x << ", " << det.y << ")" << std::endl;
  }

  std::cout << "\n=== Test 4: No detection (occlusion) ===" << std::endl;
  std::vector<Detection> frame4 = {};
  auto result4 = tracker.Update(frame4);
  std::cout << "Frame 4 results (no detections):" << std::endl;
  if (result4.empty()) {
    std::cout << "  (no tracked objects)" << std::endl;
  }
  for (const auto& [id, det] : result4) {
    std::cout << "  Track ID: " << id << ", Pos: (" << det.x << ", " << det.y << ")" << std::endl;
  }

  std::cout << "\n=== Test 5: Reappearance after occlusion ===" << std::endl;
  std::vector<Detection> frame5 = {
    {102.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0},  // object 1 reappears
    {198.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0},  // object 2 reappears
    {305.0f, 300.0f, 50.0f, 50.0f, 0.9f, 0}   // object 3 still there
  };

  auto result5 = tracker.Update(frame5);
  std::cout << "Frame 5 results:" << std::endl;
  for (const auto& [id, det] : result5) {
    std::cout << "  Track ID: " << id << ", Pos: (" << det.x << ", " << det.y << ")" << std::endl;
  }

  std::cout << "\n=== Verification ===" << std::endl;
  std::cout << "Expected behavior:" << std::endl;
  std::cout << "  Frame 1: IDs = 1, 2" << std::endl;
  std::cout << "  Frame 2: IDs = 1, 2 (same)" << std::endl;
  std::cout << "  Frame 3: IDs = 1, 2, 3 (new)" << std::endl;
  std::cout << "  Frame 4: IDs = (none)" << std::endl;
  std::cout << "  Frame 5: IDs = 1, 2, 3 (recovered)" << std::endl;

  return 0;
}
