// NOLINT: This file starts with a BOM since it contain non-ASCII characters
// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from robot_interfaces:msg/TargetInfo.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/msg/target_info.h"


#ifndef ROBOT_INTERFACES__MSG__DETAIL__TARGET_INFO__STRUCT_H_
#define ROBOT_INTERFACES__MSG__DETAIL__TARGET_INFO__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Constants defined in the message

// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__struct.h"
// Member 'class_name'
// Member 'track_id'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/TargetInfo in the package robot_interfaces.
/**
  * 当前目标选择结果，预留给更完整的行为/状态显示。
 */
typedef struct robot_interfaces__msg__TargetInfo
{
  std_msgs__msg__Header header;
  /// 目标类别名和跟踪 ID。
  rosidl_runtime_c__String class_name;
  rosidl_runtime_c__String track_id;
  /// 当前目标置信度和检测框像素位置。
  float confidence;
  int32_t bounding_box_center_x;
  int32_t bounding_box_center_y;
  int32_t bounding_box_width;
  int32_t bounding_box_height;
  /// true 表示当前行为节点已经锁定一个有效目标。
  bool is_locked;
} robot_interfaces__msg__TargetInfo;

// Struct for a sequence of robot_interfaces__msg__TargetInfo.
typedef struct robot_interfaces__msg__TargetInfo__Sequence
{
  robot_interfaces__msg__TargetInfo * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} robot_interfaces__msg__TargetInfo__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // ROBOT_INTERFACES__MSG__DETAIL__TARGET_INFO__STRUCT_H_
