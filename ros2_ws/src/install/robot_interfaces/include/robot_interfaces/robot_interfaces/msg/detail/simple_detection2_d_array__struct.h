// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from robot_interfaces:msg/SimpleDetection2DArray.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/msg/simple_detection2_d_array.h"


#ifndef ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION2_D_ARRAY__STRUCT_H_
#define ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION2_D_ARRAY__STRUCT_H_

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
// Member 'detections'
#include "robot_interfaces/msg/detail/simple_detection__struct.h"

/// Struct defined in msg/SimpleDetection2DArray in the package robot_interfaces.
typedef struct robot_interfaces__msg__SimpleDetection2DArray
{
  std_msgs__msg__Header header;
  robot_interfaces__msg__SimpleDetection__Sequence detections;
} robot_interfaces__msg__SimpleDetection2DArray;

// Struct for a sequence of robot_interfaces__msg__SimpleDetection2DArray.
typedef struct robot_interfaces__msg__SimpleDetection2DArray__Sequence
{
  robot_interfaces__msg__SimpleDetection2DArray * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} robot_interfaces__msg__SimpleDetection2DArray__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION2_D_ARRAY__STRUCT_H_
