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
typedef struct robot_interfaces__msg__TargetInfo
{
  std_msgs__msg__Header header;
  rosidl_runtime_c__String class_name;
  rosidl_runtime_c__String track_id;
  float confidence;
  int32_t bbox_cx;
  int32_t bbox_cy;
  int32_t bbox_w;
  int32_t bbox_h;
  bool locked;
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
