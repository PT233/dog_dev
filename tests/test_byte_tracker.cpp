// 单元测试：tracker_node/ByteTracker
//
// 编译（在 tests/ 目录下执行）：
//   g++ test_byte_tracker.cpp \
//       -I../ros2_ws/src/tracker_node/include \
//       ../ros2_ws/src/tracker_node/src/byte_tracker.cpp \
//       -o test_byte_tracker && ./test_byte_tracker

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
#include "tracker_node/byte_tracker.hpp"

using namespace tracker_node;

// 创建测试用检测框（中心坐标格式）
static Detection MakeDet(float x, float y, float w, float h,
                          float conf = 0.9f, int cls = 0) {
    Detection d;
    d.x = x; d.y = y; d.w = w; d.h = h;
    d.conf = conf; d.class_id = cls;
    return d;
}

// 测试 1：单帧单检测 → 创建新轨迹，ID 从 1 开始
static void test_single_detection_creates_new_track() {
    ByteTracker tracker;
    auto result = tracker.Update({MakeDet(100, 100, 50, 50)});

    assert(result.size() == 1);
    assert(result[0].first == 1);  // 第一个轨迹 ID = 1
    printf("PASS test_single_detection_creates_new_track\n");
}

// 测试 2：连续两帧同位置目标应保持同一 ID（高 IoU → 匹配成功）
static void test_track_id_persists_across_frames() {
    ByteTracker tracker;
    auto d = MakeDet(100, 100, 60, 60);

    auto r1 = tracker.Update({d});
    int id1 = r1[0].first;

    auto r2 = tracker.Update({d});  // 完全相同位置，IoU=1
    assert(r2.size() == 1);
    assert(r2[0].first == id1);  // ID 不变
    printf("PASS test_track_id_persists_across_frames\n");
}

// 测试 3：目标消失超过 track_buffer 帧后被删除，重现时分配新 ID
static void test_track_deleted_after_miss_buffer() {
    ByteTracker tracker(/*track_buffer=*/3);
    auto d = MakeDet(100, 100, 50, 50);

    auto r1 = tracker.Update({d});
    int original_id = r1[0].first;

    // 连续 4 帧空检测（超过 buffer=3 帧）
    for (int i = 0; i < 4; i++) {
        tracker.Update({});
    }

    // 目标重新出现
    auto r2 = tracker.Update({d});
    assert(r2.size() == 1);
    assert(r2[0].first != original_id);  // 旧轨迹已删除，分配新 ID
    printf("PASS test_track_deleted_after_miss_buffer\n");
}

// 测试 4：目标消失帧数未超过 buffer 时，重现后恢复原 ID
static void test_track_survives_within_buffer() {
    ByteTracker tracker(/*track_buffer=*/5);
    auto d = MakeDet(100, 100, 50, 50);

    auto r1 = tracker.Update({d});
    int original_id = r1[0].first;

    // 消失 3 帧（小于 buffer=5）
    tracker.Update({});
    tracker.Update({});
    tracker.Update({});

    // 目标重新出现，应匹配到原有轨迹
    auto r2 = tracker.Update({d});
    assert(r2.size() == 1);
    assert(r2[0].first == original_id);  // 同一 ID 恢复
    printf("PASS test_track_survives_within_buffer\n");
}

// 测试 5：两个不重叠的目标应分配不同 ID
static void test_multiple_targets_get_different_ids() {
    ByteTracker tracker;
    auto result = tracker.Update({
        MakeDet(100, 100, 50, 50),
        MakeDet(500, 500, 50, 50)   // 与第一个完全不重叠
    });

    assert(result.size() == 2);
    assert(result[0].first != result[1].first);  // 不同 ID
    printf("PASS test_multiple_targets_get_different_ids\n");
}

// 测试 6：置信度低于 track_thresh 的检测不应创建轨迹
static void test_low_confidence_detection_filtered() {
    ByteTracker tracker(30, /*track_thresh=*/0.5f);
    auto result = tracker.Update({MakeDet(100, 100, 50, 50, /*conf=*/0.3f)});

    assert(result.empty());  // 0.3 < 0.5，被过滤
    printf("PASS test_low_confidence_detection_filtered\n");
}

// 测试 7：目标小幅移动后仍应匹配到同一 ID（IoU 仍高于匹配阈值）
static void test_track_matches_after_small_motion() {
    ByteTracker tracker;

    auto r1 = tracker.Update({MakeDet(100, 100, 80, 80)});
    int id1 = r1[0].first;

    // 轻微移动 10px（bbox 重叠 >50%）
    auto r2 = tracker.Update({MakeDet(110, 110, 80, 80)});
    assert(r2.size() == 1);
    assert(r2[0].first == id1);  // 应匹配到原轨迹
    printf("PASS test_track_matches_after_small_motion\n");
}

// 测试 8：大幅移动后 IoU=0，应创建新轨迹（旧轨迹进入 miss 状态）
static void test_large_motion_creates_new_track() {
    ByteTracker tracker(30, 0.5f, /*match_thresh=*/0.5f);

    auto r1 = tracker.Update({MakeDet(100, 100, 50, 50)});
    int id1 = r1[0].first;

    // 移动到完全不重叠的位置（IoU=0 → 1-IoU=1 > match_thresh=0.5，不匹配）
    auto r2 = tracker.Update({MakeDet(600, 600, 50, 50)});
    assert(r2.size() == 1);
    assert(r2[0].first != id1);  // 新 ID
    printf("PASS test_large_motion_creates_new_track\n");
}

// 测试 9：Reset() 清空所有轨迹，下一帧从 ID=1 重新开始
static void test_reset_clears_all_tracks() {
    ByteTracker tracker;
    tracker.Update({MakeDet(100, 100, 50, 50)});
    tracker.Update({MakeDet(200, 200, 50, 50)});

    tracker.Reset();

    auto result = tracker.Update({MakeDet(100, 100, 50, 50)});
    assert(result.size() == 1);
    assert(result[0].first == 1);  // Reset 后 ID 从 1 重新分配
    printf("PASS test_reset_clears_all_tracks\n");
}

// 测试 10：IoU 计算正确性验证（完全重叠 → IoU=1，完全分离 → IoU=0）
// ByteTracker 内部以 1-IoU 作为代价，IoU=1 代价=0 必然匹配
static void test_perfect_overlap_always_matches() {
    ByteTracker tracker;
    auto d = MakeDet(200, 200, 100, 100);

    auto r1 = tracker.Update({d});
    auto r2 = tracker.Update({d});  // 完全重叠

    assert(r2[0].first == r1[0].first);
    printf("PASS test_perfect_overlap_always_matches\n");
}

int main() {
    printf("=== ByteTracker Unit Tests ===\n");
    test_single_detection_creates_new_track();
    test_track_id_persists_across_frames();
    test_track_deleted_after_miss_buffer();
    test_track_survives_within_buffer();
    test_multiple_targets_get_different_ids();
    test_low_confidence_detection_filtered();
    test_track_matches_after_small_motion();
    test_large_motion_creates_new_track();
    test_reset_clears_all_tracks();
    test_perfect_overlap_always_matches();
    printf("=== All tests PASSED ===\n");
    return 0;
}
