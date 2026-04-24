// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from robot_interfaces:msg/SimpleDetection.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/msg/simple_detection.h"


#ifndef ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION__STRUCT_H_
#define ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Constants defined in the message

// Include directives for member types
// Member 'track_id'
#include "rosidl_runtime_c/string.h"

/// Struct defined in msg/SimpleDetection in the package robot_interfaces.
typedef struct robot_interfaces__msg__SimpleDetection
{
  float center_x;
  float center_y;
  float width;
  float height;
  float confidence;
  int32_t class_id;
  rosidl_runtime_c__String track_id;
} robot_interfaces__msg__SimpleDetection;

// Struct for a sequence of robot_interfaces__msg__SimpleDetection.
typedef struct robot_interfaces__msg__SimpleDetection__Sequence
{
  robot_interfaces__msg__SimpleDetection * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} robot_interfaces__msg__SimpleDetection__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION__STRUCT_H_
