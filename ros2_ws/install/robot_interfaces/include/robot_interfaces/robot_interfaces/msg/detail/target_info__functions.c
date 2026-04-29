// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from robot_interfaces:msg/TargetInfo.idl
// generated code does not contain a copyright notice
#include "robot_interfaces/msg/detail/target_info__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `header`
#include "std_msgs/msg/detail/header__functions.h"
// Member `class_name`
// Member `track_id`
#include "rosidl_runtime_c/string_functions.h"

bool
robot_interfaces__msg__TargetInfo__init(robot_interfaces__msg__TargetInfo * msg)
{
  if (!msg) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__init(&msg->header)) {
    robot_interfaces__msg__TargetInfo__fini(msg);
    return false;
  }
  // class_name
  if (!rosidl_runtime_c__String__init(&msg->class_name)) {
    robot_interfaces__msg__TargetInfo__fini(msg);
    return false;
  }
  // track_id
  if (!rosidl_runtime_c__String__init(&msg->track_id)) {
    robot_interfaces__msg__TargetInfo__fini(msg);
    return false;
  }
  // confidence
  // bounding_box_center_x
  // bounding_box_center_y
  // bounding_box_width
  // bounding_box_height
  // is_locked
  return true;
}

void
robot_interfaces__msg__TargetInfo__fini(robot_interfaces__msg__TargetInfo * msg)
{
  if (!msg) {
    return;
  }
  // header
  std_msgs__msg__Header__fini(&msg->header);
  // class_name
  rosidl_runtime_c__String__fini(&msg->class_name);
  // track_id
  rosidl_runtime_c__String__fini(&msg->track_id);
  // confidence
  // bounding_box_center_x
  // bounding_box_center_y
  // bounding_box_width
  // bounding_box_height
  // is_locked
}

bool
robot_interfaces__msg__TargetInfo__are_equal(const robot_interfaces__msg__TargetInfo * lhs, const robot_interfaces__msg__TargetInfo * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__are_equal(
      &(lhs->header), &(rhs->header)))
  {
    return false;
  }
  // class_name
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->class_name), &(rhs->class_name)))
  {
    return false;
  }
  // track_id
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->track_id), &(rhs->track_id)))
  {
    return false;
  }
  // confidence
  if (lhs->confidence != rhs->confidence) {
    return false;
  }
  // bounding_box_center_x
  if (lhs->bounding_box_center_x != rhs->bounding_box_center_x) {
    return false;
  }
  // bounding_box_center_y
  if (lhs->bounding_box_center_y != rhs->bounding_box_center_y) {
    return false;
  }
  // bounding_box_width
  if (lhs->bounding_box_width != rhs->bounding_box_width) {
    return false;
  }
  // bounding_box_height
  if (lhs->bounding_box_height != rhs->bounding_box_height) {
    return false;
  }
  // is_locked
  if (lhs->is_locked != rhs->is_locked) {
    return false;
  }
  return true;
}

bool
robot_interfaces__msg__TargetInfo__copy(
  const robot_interfaces__msg__TargetInfo * input,
  robot_interfaces__msg__TargetInfo * output)
{
  if (!input || !output) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__copy(
      &(input->header), &(output->header)))
  {
    return false;
  }
  // class_name
  if (!rosidl_runtime_c__String__copy(
      &(input->class_name), &(output->class_name)))
  {
    return false;
  }
  // track_id
  if (!rosidl_runtime_c__String__copy(
      &(input->track_id), &(output->track_id)))
  {
    return false;
  }
  // confidence
  output->confidence = input->confidence;
  // bounding_box_center_x
  output->bounding_box_center_x = input->bounding_box_center_x;
  // bounding_box_center_y
  output->bounding_box_center_y = input->bounding_box_center_y;
  // bounding_box_width
  output->bounding_box_width = input->bounding_box_width;
  // bounding_box_height
  output->bounding_box_height = input->bounding_box_height;
  // is_locked
  output->is_locked = input->is_locked;
  return true;
}

robot_interfaces__msg__TargetInfo *
robot_interfaces__msg__TargetInfo__create(void)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__msg__TargetInfo * msg = (robot_interfaces__msg__TargetInfo *)allocator.allocate(sizeof(robot_interfaces__msg__TargetInfo), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(robot_interfaces__msg__TargetInfo));
  bool success = robot_interfaces__msg__TargetInfo__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
robot_interfaces__msg__TargetInfo__destroy(robot_interfaces__msg__TargetInfo * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    robot_interfaces__msg__TargetInfo__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
robot_interfaces__msg__TargetInfo__Sequence__init(robot_interfaces__msg__TargetInfo__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__msg__TargetInfo * data = NULL;

  if (size) {
    data = (robot_interfaces__msg__TargetInfo *)allocator.zero_allocate(size, sizeof(robot_interfaces__msg__TargetInfo), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = robot_interfaces__msg__TargetInfo__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        robot_interfaces__msg__TargetInfo__fini(&data[i - 1]);
      }
      allocator.deallocate(data, allocator.state);
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
robot_interfaces__msg__TargetInfo__Sequence__fini(robot_interfaces__msg__TargetInfo__Sequence * array)
{
  if (!array) {
    return;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    for (size_t i = 0; i < array->capacity; ++i) {
      robot_interfaces__msg__TargetInfo__fini(&array->data[i]);
    }
    allocator.deallocate(array->data, allocator.state);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

robot_interfaces__msg__TargetInfo__Sequence *
robot_interfaces__msg__TargetInfo__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__msg__TargetInfo__Sequence * array = (robot_interfaces__msg__TargetInfo__Sequence *)allocator.allocate(sizeof(robot_interfaces__msg__TargetInfo__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = robot_interfaces__msg__TargetInfo__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
robot_interfaces__msg__TargetInfo__Sequence__destroy(robot_interfaces__msg__TargetInfo__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    robot_interfaces__msg__TargetInfo__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
robot_interfaces__msg__TargetInfo__Sequence__are_equal(const robot_interfaces__msg__TargetInfo__Sequence * lhs, const robot_interfaces__msg__TargetInfo__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!robot_interfaces__msg__TargetInfo__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
robot_interfaces__msg__TargetInfo__Sequence__copy(
  const robot_interfaces__msg__TargetInfo__Sequence * input,
  robot_interfaces__msg__TargetInfo__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(robot_interfaces__msg__TargetInfo);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    robot_interfaces__msg__TargetInfo * data =
      (robot_interfaces__msg__TargetInfo *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!robot_interfaces__msg__TargetInfo__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          robot_interfaces__msg__TargetInfo__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!robot_interfaces__msg__TargetInfo__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
