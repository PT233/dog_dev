// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from robot_interfaces:srv/SetTargetClass.idl
// generated code does not contain a copyright notice
#include "robot_interfaces/srv/detail/set_target_class__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"

// Include directives for member types
// Member `class_name`
#include "rosidl_runtime_c/string_functions.h"

bool
robot_interfaces__srv__SetTargetClass_Request__init(robot_interfaces__srv__SetTargetClass_Request * msg)
{
  if (!msg) {
    return false;
  }
  // class_name
  if (!rosidl_runtime_c__String__init(&msg->class_name)) {
    robot_interfaces__srv__SetTargetClass_Request__fini(msg);
    return false;
  }
  return true;
}

void
robot_interfaces__srv__SetTargetClass_Request__fini(robot_interfaces__srv__SetTargetClass_Request * msg)
{
  if (!msg) {
    return;
  }
  // class_name
  rosidl_runtime_c__String__fini(&msg->class_name);
}

bool
robot_interfaces__srv__SetTargetClass_Request__are_equal(const robot_interfaces__srv__SetTargetClass_Request * lhs, const robot_interfaces__srv__SetTargetClass_Request * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // class_name
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->class_name), &(rhs->class_name)))
  {
    return false;
  }
  return true;
}

bool
robot_interfaces__srv__SetTargetClass_Request__copy(
  const robot_interfaces__srv__SetTargetClass_Request * input,
  robot_interfaces__srv__SetTargetClass_Request * output)
{
  if (!input || !output) {
    return false;
  }
  // class_name
  if (!rosidl_runtime_c__String__copy(
      &(input->class_name), &(output->class_name)))
  {
    return false;
  }
  return true;
}

robot_interfaces__srv__SetTargetClass_Request *
robot_interfaces__srv__SetTargetClass_Request__create(void)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__srv__SetTargetClass_Request * msg = (robot_interfaces__srv__SetTargetClass_Request *)allocator.allocate(sizeof(robot_interfaces__srv__SetTargetClass_Request), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(robot_interfaces__srv__SetTargetClass_Request));
  bool success = robot_interfaces__srv__SetTargetClass_Request__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
robot_interfaces__srv__SetTargetClass_Request__destroy(robot_interfaces__srv__SetTargetClass_Request * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    robot_interfaces__srv__SetTargetClass_Request__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
robot_interfaces__srv__SetTargetClass_Request__Sequence__init(robot_interfaces__srv__SetTargetClass_Request__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__srv__SetTargetClass_Request * data = NULL;

  if (size) {
    data = (robot_interfaces__srv__SetTargetClass_Request *)allocator.zero_allocate(size, sizeof(robot_interfaces__srv__SetTargetClass_Request), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = robot_interfaces__srv__SetTargetClass_Request__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        robot_interfaces__srv__SetTargetClass_Request__fini(&data[i - 1]);
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
robot_interfaces__srv__SetTargetClass_Request__Sequence__fini(robot_interfaces__srv__SetTargetClass_Request__Sequence * array)
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
      robot_interfaces__srv__SetTargetClass_Request__fini(&array->data[i]);
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

robot_interfaces__srv__SetTargetClass_Request__Sequence *
robot_interfaces__srv__SetTargetClass_Request__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__srv__SetTargetClass_Request__Sequence * array = (robot_interfaces__srv__SetTargetClass_Request__Sequence *)allocator.allocate(sizeof(robot_interfaces__srv__SetTargetClass_Request__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = robot_interfaces__srv__SetTargetClass_Request__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
robot_interfaces__srv__SetTargetClass_Request__Sequence__destroy(robot_interfaces__srv__SetTargetClass_Request__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    robot_interfaces__srv__SetTargetClass_Request__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
robot_interfaces__srv__SetTargetClass_Request__Sequence__are_equal(const robot_interfaces__srv__SetTargetClass_Request__Sequence * lhs, const robot_interfaces__srv__SetTargetClass_Request__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!robot_interfaces__srv__SetTargetClass_Request__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
robot_interfaces__srv__SetTargetClass_Request__Sequence__copy(
  const robot_interfaces__srv__SetTargetClass_Request__Sequence * input,
  robot_interfaces__srv__SetTargetClass_Request__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(robot_interfaces__srv__SetTargetClass_Request);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    robot_interfaces__srv__SetTargetClass_Request * data =
      (robot_interfaces__srv__SetTargetClass_Request *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!robot_interfaces__srv__SetTargetClass_Request__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          robot_interfaces__srv__SetTargetClass_Request__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!robot_interfaces__srv__SetTargetClass_Request__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}


// Include directives for member types
// Member `message`
// already included above
// #include "rosidl_runtime_c/string_functions.h"

bool
robot_interfaces__srv__SetTargetClass_Response__init(robot_interfaces__srv__SetTargetClass_Response * msg)
{
  if (!msg) {
    return false;
  }
  // success
  // message
  if (!rosidl_runtime_c__String__init(&msg->message)) {
    robot_interfaces__srv__SetTargetClass_Response__fini(msg);
    return false;
  }
  return true;
}

void
robot_interfaces__srv__SetTargetClass_Response__fini(robot_interfaces__srv__SetTargetClass_Response * msg)
{
  if (!msg) {
    return;
  }
  // success
  // message
  rosidl_runtime_c__String__fini(&msg->message);
}

bool
robot_interfaces__srv__SetTargetClass_Response__are_equal(const robot_interfaces__srv__SetTargetClass_Response * lhs, const robot_interfaces__srv__SetTargetClass_Response * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // success
  if (lhs->success != rhs->success) {
    return false;
  }
  // message
  if (!rosidl_runtime_c__String__are_equal(
      &(lhs->message), &(rhs->message)))
  {
    return false;
  }
  return true;
}

bool
robot_interfaces__srv__SetTargetClass_Response__copy(
  const robot_interfaces__srv__SetTargetClass_Response * input,
  robot_interfaces__srv__SetTargetClass_Response * output)
{
  if (!input || !output) {
    return false;
  }
  // success
  output->success = input->success;
  // message
  if (!rosidl_runtime_c__String__copy(
      &(input->message), &(output->message)))
  {
    return false;
  }
  return true;
}

robot_interfaces__srv__SetTargetClass_Response *
robot_interfaces__srv__SetTargetClass_Response__create(void)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__srv__SetTargetClass_Response * msg = (robot_interfaces__srv__SetTargetClass_Response *)allocator.allocate(sizeof(robot_interfaces__srv__SetTargetClass_Response), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(robot_interfaces__srv__SetTargetClass_Response));
  bool success = robot_interfaces__srv__SetTargetClass_Response__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
robot_interfaces__srv__SetTargetClass_Response__destroy(robot_interfaces__srv__SetTargetClass_Response * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    robot_interfaces__srv__SetTargetClass_Response__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
robot_interfaces__srv__SetTargetClass_Response__Sequence__init(robot_interfaces__srv__SetTargetClass_Response__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__srv__SetTargetClass_Response * data = NULL;

  if (size) {
    data = (robot_interfaces__srv__SetTargetClass_Response *)allocator.zero_allocate(size, sizeof(robot_interfaces__srv__SetTargetClass_Response), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = robot_interfaces__srv__SetTargetClass_Response__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        robot_interfaces__srv__SetTargetClass_Response__fini(&data[i - 1]);
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
robot_interfaces__srv__SetTargetClass_Response__Sequence__fini(robot_interfaces__srv__SetTargetClass_Response__Sequence * array)
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
      robot_interfaces__srv__SetTargetClass_Response__fini(&array->data[i]);
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

robot_interfaces__srv__SetTargetClass_Response__Sequence *
robot_interfaces__srv__SetTargetClass_Response__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__srv__SetTargetClass_Response__Sequence * array = (robot_interfaces__srv__SetTargetClass_Response__Sequence *)allocator.allocate(sizeof(robot_interfaces__srv__SetTargetClass_Response__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = robot_interfaces__srv__SetTargetClass_Response__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
robot_interfaces__srv__SetTargetClass_Response__Sequence__destroy(robot_interfaces__srv__SetTargetClass_Response__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    robot_interfaces__srv__SetTargetClass_Response__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
robot_interfaces__srv__SetTargetClass_Response__Sequence__are_equal(const robot_interfaces__srv__SetTargetClass_Response__Sequence * lhs, const robot_interfaces__srv__SetTargetClass_Response__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!robot_interfaces__srv__SetTargetClass_Response__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
robot_interfaces__srv__SetTargetClass_Response__Sequence__copy(
  const robot_interfaces__srv__SetTargetClass_Response__Sequence * input,
  robot_interfaces__srv__SetTargetClass_Response__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(robot_interfaces__srv__SetTargetClass_Response);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    robot_interfaces__srv__SetTargetClass_Response * data =
      (robot_interfaces__srv__SetTargetClass_Response *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!robot_interfaces__srv__SetTargetClass_Response__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          robot_interfaces__srv__SetTargetClass_Response__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!robot_interfaces__srv__SetTargetClass_Response__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}


// Include directives for member types
// Member `info`
#include "service_msgs/msg/detail/service_event_info__functions.h"
// Member `request`
// Member `response`
// already included above
// #include "robot_interfaces/srv/detail/set_target_class__functions.h"

bool
robot_interfaces__srv__SetTargetClass_Event__init(robot_interfaces__srv__SetTargetClass_Event * msg)
{
  if (!msg) {
    return false;
  }
  // info
  if (!service_msgs__msg__ServiceEventInfo__init(&msg->info)) {
    robot_interfaces__srv__SetTargetClass_Event__fini(msg);
    return false;
  }
  // request
  if (!robot_interfaces__srv__SetTargetClass_Request__Sequence__init(&msg->request, 0)) {
    robot_interfaces__srv__SetTargetClass_Event__fini(msg);
    return false;
  }
  // response
  if (!robot_interfaces__srv__SetTargetClass_Response__Sequence__init(&msg->response, 0)) {
    robot_interfaces__srv__SetTargetClass_Event__fini(msg);
    return false;
  }
  return true;
}

void
robot_interfaces__srv__SetTargetClass_Event__fini(robot_interfaces__srv__SetTargetClass_Event * msg)
{
  if (!msg) {
    return;
  }
  // info
  service_msgs__msg__ServiceEventInfo__fini(&msg->info);
  // request
  robot_interfaces__srv__SetTargetClass_Request__Sequence__fini(&msg->request);
  // response
  robot_interfaces__srv__SetTargetClass_Response__Sequence__fini(&msg->response);
}

bool
robot_interfaces__srv__SetTargetClass_Event__are_equal(const robot_interfaces__srv__SetTargetClass_Event * lhs, const robot_interfaces__srv__SetTargetClass_Event * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // info
  if (!service_msgs__msg__ServiceEventInfo__are_equal(
      &(lhs->info), &(rhs->info)))
  {
    return false;
  }
  // request
  if (!robot_interfaces__srv__SetTargetClass_Request__Sequence__are_equal(
      &(lhs->request), &(rhs->request)))
  {
    return false;
  }
  // response
  if (!robot_interfaces__srv__SetTargetClass_Response__Sequence__are_equal(
      &(lhs->response), &(rhs->response)))
  {
    return false;
  }
  return true;
}

bool
robot_interfaces__srv__SetTargetClass_Event__copy(
  const robot_interfaces__srv__SetTargetClass_Event * input,
  robot_interfaces__srv__SetTargetClass_Event * output)
{
  if (!input || !output) {
    return false;
  }
  // info
  if (!service_msgs__msg__ServiceEventInfo__copy(
      &(input->info), &(output->info)))
  {
    return false;
  }
  // request
  if (!robot_interfaces__srv__SetTargetClass_Request__Sequence__copy(
      &(input->request), &(output->request)))
  {
    return false;
  }
  // response
  if (!robot_interfaces__srv__SetTargetClass_Response__Sequence__copy(
      &(input->response), &(output->response)))
  {
    return false;
  }
  return true;
}

robot_interfaces__srv__SetTargetClass_Event *
robot_interfaces__srv__SetTargetClass_Event__create(void)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__srv__SetTargetClass_Event * msg = (robot_interfaces__srv__SetTargetClass_Event *)allocator.allocate(sizeof(robot_interfaces__srv__SetTargetClass_Event), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(robot_interfaces__srv__SetTargetClass_Event));
  bool success = robot_interfaces__srv__SetTargetClass_Event__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
robot_interfaces__srv__SetTargetClass_Event__destroy(robot_interfaces__srv__SetTargetClass_Event * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    robot_interfaces__srv__SetTargetClass_Event__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
robot_interfaces__srv__SetTargetClass_Event__Sequence__init(robot_interfaces__srv__SetTargetClass_Event__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__srv__SetTargetClass_Event * data = NULL;

  if (size) {
    data = (robot_interfaces__srv__SetTargetClass_Event *)allocator.zero_allocate(size, sizeof(robot_interfaces__srv__SetTargetClass_Event), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = robot_interfaces__srv__SetTargetClass_Event__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        robot_interfaces__srv__SetTargetClass_Event__fini(&data[i - 1]);
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
robot_interfaces__srv__SetTargetClass_Event__Sequence__fini(robot_interfaces__srv__SetTargetClass_Event__Sequence * array)
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
      robot_interfaces__srv__SetTargetClass_Event__fini(&array->data[i]);
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

robot_interfaces__srv__SetTargetClass_Event__Sequence *
robot_interfaces__srv__SetTargetClass_Event__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  robot_interfaces__srv__SetTargetClass_Event__Sequence * array = (robot_interfaces__srv__SetTargetClass_Event__Sequence *)allocator.allocate(sizeof(robot_interfaces__srv__SetTargetClass_Event__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = robot_interfaces__srv__SetTargetClass_Event__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
robot_interfaces__srv__SetTargetClass_Event__Sequence__destroy(robot_interfaces__srv__SetTargetClass_Event__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    robot_interfaces__srv__SetTargetClass_Event__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
robot_interfaces__srv__SetTargetClass_Event__Sequence__are_equal(const robot_interfaces__srv__SetTargetClass_Event__Sequence * lhs, const robot_interfaces__srv__SetTargetClass_Event__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!robot_interfaces__srv__SetTargetClass_Event__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
robot_interfaces__srv__SetTargetClass_Event__Sequence__copy(
  const robot_interfaces__srv__SetTargetClass_Event__Sequence * input,
  robot_interfaces__srv__SetTargetClass_Event__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(robot_interfaces__srv__SetTargetClass_Event);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    robot_interfaces__srv__SetTargetClass_Event * data =
      (robot_interfaces__srv__SetTargetClass_Event *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!robot_interfaces__srv__SetTargetClass_Event__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          robot_interfaces__srv__SetTargetClass_Event__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!robot_interfaces__srv__SetTargetClass_Event__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
