// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from robot_interfaces:msg/SimpleDetection2DArray.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/msg/simple_detection2_d_array.h"


#ifndef ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION2_D_ARRAY__FUNCTIONS_H_
#define ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION2_D_ARRAY__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/action_type_support_struct.h"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_c/service_type_support_struct.h"
#include "rosidl_runtime_c/type_description/type_description__struct.h"
#include "rosidl_runtime_c/type_description/type_source__struct.h"
#include "rosidl_runtime_c/type_hash.h"
#include "rosidl_runtime_c/visibility_control.h"
#include "robot_interfaces/msg/rosidl_generator_c__visibility_control.h"

#include "robot_interfaces/msg/detail/simple_detection2_d_array__struct.h"

/// Initialize msg/SimpleDetection2DArray message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * robot_interfaces__msg__SimpleDetection2DArray
 * )) before or use
 * robot_interfaces__msg__SimpleDetection2DArray__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
bool
robot_interfaces__msg__SimpleDetection2DArray__init(robot_interfaces__msg__SimpleDetection2DArray * msg);

/// Finalize msg/SimpleDetection2DArray message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
void
robot_interfaces__msg__SimpleDetection2DArray__fini(robot_interfaces__msg__SimpleDetection2DArray * msg);

/// Create msg/SimpleDetection2DArray message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * robot_interfaces__msg__SimpleDetection2DArray__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
robot_interfaces__msg__SimpleDetection2DArray *
robot_interfaces__msg__SimpleDetection2DArray__create(void);

/// Destroy msg/SimpleDetection2DArray message.
/**
 * It calls
 * robot_interfaces__msg__SimpleDetection2DArray__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
void
robot_interfaces__msg__SimpleDetection2DArray__destroy(robot_interfaces__msg__SimpleDetection2DArray * msg);

/// Check for msg/SimpleDetection2DArray message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
bool
robot_interfaces__msg__SimpleDetection2DArray__are_equal(const robot_interfaces__msg__SimpleDetection2DArray * lhs, const robot_interfaces__msg__SimpleDetection2DArray * rhs);

/// Copy a msg/SimpleDetection2DArray message.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source message pointer.
 * \param[out] output The target message pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer is null
 *   or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
bool
robot_interfaces__msg__SimpleDetection2DArray__copy(
  const robot_interfaces__msg__SimpleDetection2DArray * input,
  robot_interfaces__msg__SimpleDetection2DArray * output);

/// Retrieve pointer to the hash of the description of this type.
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
const rosidl_type_hash_t *
robot_interfaces__msg__SimpleDetection2DArray__get_type_hash(
  const rosidl_message_type_support_t * type_support);

/// Retrieve pointer to the description of this type.
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
const rosidl_runtime_c__type_description__TypeDescription *
robot_interfaces__msg__SimpleDetection2DArray__get_type_description(
  const rosidl_message_type_support_t * type_support);

/// Retrieve pointer to the single raw source text that defined this type.
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
const rosidl_runtime_c__type_description__TypeSource *
robot_interfaces__msg__SimpleDetection2DArray__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support);

/// Retrieve pointer to the recursive raw sources that defined the description of this type.
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
const rosidl_runtime_c__type_description__TypeSource__Sequence *
robot_interfaces__msg__SimpleDetection2DArray__get_type_description_sources(
  const rosidl_message_type_support_t * type_support);

/// Initialize array of msg/SimpleDetection2DArray messages.
/**
 * It allocates the memory for the number of elements and calls
 * robot_interfaces__msg__SimpleDetection2DArray__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
bool
robot_interfaces__msg__SimpleDetection2DArray__Sequence__init(robot_interfaces__msg__SimpleDetection2DArray__Sequence * array, size_t size);

/// Finalize array of msg/SimpleDetection2DArray messages.
/**
 * It calls
 * robot_interfaces__msg__SimpleDetection2DArray__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
void
robot_interfaces__msg__SimpleDetection2DArray__Sequence__fini(robot_interfaces__msg__SimpleDetection2DArray__Sequence * array);

/// Create array of msg/SimpleDetection2DArray messages.
/**
 * It allocates the memory for the array and calls
 * robot_interfaces__msg__SimpleDetection2DArray__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
robot_interfaces__msg__SimpleDetection2DArray__Sequence *
robot_interfaces__msg__SimpleDetection2DArray__Sequence__create(size_t size);

/// Destroy array of msg/SimpleDetection2DArray messages.
/**
 * It calls
 * robot_interfaces__msg__SimpleDetection2DArray__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
void
robot_interfaces__msg__SimpleDetection2DArray__Sequence__destroy(robot_interfaces__msg__SimpleDetection2DArray__Sequence * array);

/// Check for msg/SimpleDetection2DArray message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
bool
robot_interfaces__msg__SimpleDetection2DArray__Sequence__are_equal(const robot_interfaces__msg__SimpleDetection2DArray__Sequence * lhs, const robot_interfaces__msg__SimpleDetection2DArray__Sequence * rhs);

/// Copy an array of msg/SimpleDetection2DArray messages.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source array pointer.
 * \param[out] output The target array pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer
 *   is null or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
bool
robot_interfaces__msg__SimpleDetection2DArray__Sequence__copy(
  const robot_interfaces__msg__SimpleDetection2DArray__Sequence * input,
  robot_interfaces__msg__SimpleDetection2DArray__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION2_D_ARRAY__FUNCTIONS_H_
