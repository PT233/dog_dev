#include <gtest/gtest.h>
#include "tracker_node/byte_tracker.hpp"

using namespace tracker_node;

class TrackerNodeTest : public ::testing::Test {
protected:
  std::shared_ptr<ByteTracker> tracker;

  void SetUp() override {
    tracker = std::make_shared<ByteTracker>(30, 0.5f, 0.5f);
  }
};

TEST_F(TrackerNodeTest, ParameterLoading) {
  EXPECT_NE(tracker, nullptr);
}

TEST_F(TrackerNodeTest, BasicTracking) {
  // Frame 1: Two detections
  std::vector<Detection> frame1 = {
    {100.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0},
    {200.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0}
  };
  auto result1 = tracker->Update(frame1);
  ASSERT_EQ(result1.size(), 2);
  EXPECT_EQ(result1[0].first, 1);
  EXPECT_EQ(result1[1].first, 2);

  // Frame 2: Same detections (shifted slightly)
  std::vector<Detection> frame2 = {
    {102.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0},
    {198.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0}
  };
  auto result2 = tracker->Update(frame2);
  ASSERT_EQ(result2.size(), 2);
  EXPECT_EQ(result2[0].first, 1);
  EXPECT_EQ(result2[1].first, 2);
}

TEST_F(TrackerNodeTest, NewDetectionCreatesNewTrack) {
  std::vector<Detection> frame1 = {
    {100.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0},
    {200.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0}
  };
  tracker->Update(frame1);

  std::vector<Detection> frame2 = {
    {100.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0},
    {200.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0},
    {300.0f, 300.0f, 50.0f, 50.0f, 0.9f, 0}  // New detection
  };
  auto result2 = tracker->Update(frame2);
  ASSERT_EQ(result2.size(), 3);
  EXPECT_EQ(result2[2].first, 3);  // New ID
}

TEST_F(TrackerNodeTest, OcclusionRecovery) {
  std::vector<Detection> frame1 = {
    {100.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0},
    {200.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0}
  };
  tracker->Update(frame1);

  // No detections (occlusion)
  std::vector<Detection> frame2 = {};
  tracker->Update(frame2);

  // Reappearance
  std::vector<Detection> frame3 = {
    {100.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0},
    {200.0f, 100.0f, 50.0f, 50.0f, 0.9f, 0}
  };
  auto result3 = tracker->Update(frame3);
  ASSERT_EQ(result3.size(), 2);
  EXPECT_EQ(result3[0].first, 1);  // ID preserved
  EXPECT_EQ(result3[1].first, 2);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
