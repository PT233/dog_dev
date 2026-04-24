// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from robot_interfaces:srv/SetTargetClass.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/srv/set_target_class.h"


#ifndef ROBOT_INTERFACES__SRV__DETAIL__SET_TARGET_CLASS__STRUCT_H_
#define ROBOT_INTERFACES__SRV__DETAIL__SET_TARGET_CLASS__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

// Include directives for member types
// Member 'class_name'
#include "rosidl_runtime_c/string.h"

/// Struct defined in srv/SetTargetClass in the package robot_interfaces.
typedef struct robot_interfaces__srv__SetTargetClass_Request
{
  rosidl_runtime_c__String class_name;
} robot_interfaces__srv__SetTargetClass_Request;

// Struct for a sequence of robot_interfaces__srv__SetTargetClass_Request.
typedef struct robot_interfaces__srv__SetTargetClass_Request__Sequence
{
  robot_interfaces__srv__SetTargetClass_Request * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} robot_interfaces__srv__SetTargetClass_Request__Sequence;

// Constants defined in the message

// Include directives for member types
// Member 'message'
// already included above
// #include "rosidl_runtime_c/string.h"

/// Struct defined in srv/SetTargetClass in the package robot_interfaces.
typedef struct robot_interfaces__srv__SetTargetClass_Response
{
  bool success;
  rosidl_runtime_c__String message;
} robot_interfaces__srv__SetTargetClass_Response;

// Struct for a sequence of robot_interfaces__srv__SetTargetClass_Response.
typedef struct robot_interfaces__srv__SetTargetClass_Response__Sequence
{
  robot_interfaces__srv__SetTargetClass_Response * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} robot_interfaces__srv__SetTargetClass_Response__Sequence;

// Constants defined in the message

// Include directives for member types
// Member 'info'
#include "service_msgs/msg/detail/service_event_info__struct.h"

// constants for array fields with an upper bound
// request
enum
{
  robot_interfaces__srv__SetTargetClass_Event__request__MAX_SIZE = 1
};
// response
enum
{
  robot_interfaces__srv__SetTargetClass_Event__response__MAX_SIZE = 1
};

/// Struct defined in srv/SetTargetClass in the package robot_interfaces.
typedef struct robot_interfaces__srv__SetTargetClass_Event
{
  service_msgs__msg__ServiceEventInfo info;
  robot_interfaces__srv__SetTargetClass_Request__Sequence request;
  robot_interfaces__srv__SetTargetClass_Response__Sequence response;
} robot_interfaces__srv__SetTargetClass_Event;

// Struct for a sequence of robot_interfaces__srv__SetTargetClass_Event.
typedef struct robot_interfaces__srv__SetTargetClass_Event__Sequence
{
  robot_interfaces__srv__SetTargetClass_Event * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} robot_interfaces__srv__SetTargetClass_Event__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // ROBOT_INTERFACES__SRV__DETAIL__SET_TARGET_CLASS__STRUCT_H_
