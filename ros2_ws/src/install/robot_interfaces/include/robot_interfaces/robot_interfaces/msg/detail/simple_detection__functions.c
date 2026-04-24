// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from robot_interfaces:msg/SimpleDetection.idl
// generated code does not contain a copyright notice
#include "robot_interfaces/msg/detail/simple_detection__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `track_id`
#include "rosidl_runtime_c/string_functions.h"

bool
robot_interfaces__msg__SimpleDetection__init(robot_interfaces__msg__SimpleDetection * msg)
{
  if (!msg) {
    return false;
  }
  // center_x
  // center_y
  // width
  // height
  // confidence
  // class_id
  // track_id
  if (!rosidl_runtime_c__String__init(&msg->track_id)) {
    robot_interfaces__msg__SimpleDetection__fini(msg);
    return false;
  }
  return true;
}

void
robot_interfaces__msg__SimpleDetection__fini(robot_interfaces__msg__SimpleDetection * msg)
{
  if (!msg) {
    return;
  }
  // center_x
  // center_y
  // width
  // height
  // confidence
  // class_id
  // track_id
  rosidl_runtime_c__String__fini(&msg->track_id);
}

bool
robot_interfaces__msg__SimpleDetection__are_equal(const robot_interfaces__msg__SimpleDetection * lhs, const robot_interfaces__msg__SimpleDetection * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // center_x
  if (lhs->center_x != rhs->center_x) {
    return false;
  }
  // center_y
  if (lhs->center_y != rhs->center_y) {
    return false;
  }
  // width
  if (lhs->width != rhs->width) {
    return false;
  }
  // height
  if (lhs->height != rhs->height) {
    return false;
  }
  // confidence
  if (lhs->confidence != rhs->confidence) {
    return false;
  }
  // class_id
  if (lhs->class_id != rhs->class_id) {
    return false;
  }
  // track_id
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->track_id), &(rhs->track_id)))
  {
    return false;
  }
  return true;
}

bool
robot_interfaces__msg__SimpleDetection__copy(
  const robot_interfaces__msg__SimpleDetection * input,
  robot_interfaces__msg__SimpleDetection * output)
{
  if (!input || !output) {
    return false;
  }
  // center_x
  output->center_x = input->center_x;
  // center_y
  output->center_y = input->center_y;
  // width
  output->width = input->width;
  // height
  output->height = input->height;
  // confidence
  output->confidence = input->confidence;
  // class_id
  output->class_id = input->class_id;
  // track_id
  if (!rosidl_runtime_c__String__copy(
      &(input->track_id), &(output->track_id)))
  {
    return false;
  }
  return true;
}

robot_interfaces__msg__SimpleDetection *
robot_interfaces__msg__SimpleDetection__create(void)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__msg__SimpleDetection * msg = (robot_interfaces__msg__SimpleDetection *)allocator.allocate(sizeof(robot_interfaces__msg__SimpleDetection), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(robot_interfaces__msg__SimpleDetection));
  bool success = robot_interfaces__msg__SimpleDetection__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
robot_interfaces__msg__SimpleDetection__destroy(robot_interfaces__msg__SimpleDetection * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    robot_interfaces__msg__SimpleDetection__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
robot_interfaces__msg__SimpleDetection__Sequence__init(robot_interfaces__msg__SimpleDetection__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__msg__SimpleDetection * data = NULL;

  if (size) {
    data = (robot_interfaces__msg__SimpleDetection *)allocator.zero_allocate(size, sizeof(robot_interfaces__msg__SimpleDetection), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = robot_interfaces__msg__SimpleDetection__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        robot_interfaces__msg__SimpleDetection__fini(&data[i - 1]);
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
robot_interfaces__msg__SimpleDetection__Sequence__fini(robot_interfaces__msg__SimpleDetection__Sequence * array)
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
      robot_interfaces__msg__SimpleDetection__fini(&array->data[i]);
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

robot_interfaces__msg__SimpleDetection__Sequence *
robot_interfaces__msg__SimpleDetection__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__msg__SimpleDetection__Sequence * array = (robot_interfaces__msg__SimpleDetection__Sequence *)allocator.allocate(sizeof(robot_interfaces__msg__SimpleDetection__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = robot_interfaces__msg__SimpleDetection__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
robot_interfaces__msg__SimpleDetection__Sequence__destroy(robot_interfaces__msg__SimpleDetection__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    robot_interfaces__msg__SimpleDetection__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
robot_interfaces__msg__SimpleDetection__Sequence__are_equal(const robot_interfaces__msg__SimpleDetection__Sequence * lhs, const robot_interfaces__msg__SimpleDetection__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!robot_interfaces__msg__SimpleDetection__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
robot_interfaces__msg__SimpleDetection__Sequence__copy(
  const robot_interfaces__msg__SimpleDetection__Sequence * input,
  robot_interfaces__msg__SimpleDetection__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(robot_interfaces__msg__SimpleDetection);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    robot_interfaces__msg__SimpleDetection * data =
      (robot_interfaces__msg__SimpleDetection *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!robot_interfaces__msg__SimpleDetection__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          robot_interfaces__msg__SimpleDetection__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!robot_interfaces__msg__SimpleDetection__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
