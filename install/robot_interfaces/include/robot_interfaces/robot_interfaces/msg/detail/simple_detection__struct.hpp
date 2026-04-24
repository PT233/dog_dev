// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from robot_interfaces:msg/SimpleDetection.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/msg/simple_detection.hpp"


#ifndef ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION__STRUCT_HPP_
#define ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__robot_interfaces__msg__SimpleDetection __attribute__((deprecated))
#else
# define DEPRECATED__robot_interfaces__msg__SimpleDetection __declspec(deprecated)
#endif

namespace robot_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct SimpleDetection_
{
  using Type = SimpleDetection_<ContainerAllocator>;

  explicit SimpleDetection_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->center_x = 0.0f;
      this->center_y = 0.0f;
      this->width = 0.0f;
      this->height = 0.0f;
      this->confidence = 0.0f;
      this->class_id = 0l;
      this->track_id = "";
    }
  }

  explicit SimpleDetection_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : track_id(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->center_x = 0.0f;
      this->center_y = 0.0f;
      this->width = 0.0f;
      this->height = 0.0f;
      this->confidence = 0.0f;
      this->class_id = 0l;
      this->track_id = "";
    }
  }

  // field types and members
  using _center_x_type =
    float;
  _center_x_type center_x;
  using _center_y_type =
    float;
  _center_y_type center_y;
  using _width_type =
    float;
  _width_type width;
  using _height_type =
    float;
  _height_type height;
  using _confidence_type =
    float;
  _confidence_type confidence;
  using _class_id_type =
    int32_t;
  _class_id_type class_id;
  using _track_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _track_id_type track_id;

  // setters for named parameter idiom
  Type & set__center_x(
    const float & _arg)
  {
    this->center_x = _arg;
    return *this;
  }
  Type & set__center_y(
    const float & _arg)
  {
    this->center_y = _arg;
    return *this;
  }
  Type & set__width(
    const float & _arg)
  {
    this->width = _arg;
    return *this;
  }
  Type & set__height(
    const float & _arg)
  {
    this->height = _arg;
    return *this;
  }
  Type & set__confidence(
    const float & _arg)
  {
    this->confidence = _arg;
    return *this;
  }
  Type & set__class_id(
    const int32_t & _arg)
  {
    this->class_id = _arg;
    return *this;
  }
  Type & set__track_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->track_id = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    robot_interfaces::msg::SimpleDetection_<ContainerAllocator> *;
  using ConstRawPtr =
    const robot_interfaces::msg::SimpleDetection_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<robot_interfaces::msg::SimpleDetection_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<robot_interfaces::msg::SimpleDetection_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      robot_interfaces::msg::SimpleDetection_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<robot_interfaces::msg::SimpleDetection_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      robot_interfaces::msg::SimpleDetection_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<robot_interfaces::msg::SimpleDetection_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<robot_interfaces::msg::SimpleDetection_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<robot_interfaces::msg::SimpleDetection_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__robot_interfaces__msg__SimpleDetection
    std::shared_ptr<robot_interfaces::msg::SimpleDetection_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__robot_interfaces__msg__SimpleDetection
    std::shared_ptr<robot_interfaces::msg::SimpleDetection_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const SimpleDetection_ & other) const
  {
    if (this->center_x != other.center_x) {
      return false;
    }
    if (this->center_y != other.center_y) {
      return false;
    }
    if (this->width != other.width) {
      return false;
    }
    if (this->height != other.height) {
      return false;
    }
    if (this->confidence != other.confidence) {
      return false;
    }
    if (this->class_id != other.class_id) {
      return false;
    }
    if (this->track_id != other.track_id) {
      return false;
    }
    return true;
  }
  bool operator!=(const SimpleDetection_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct SimpleDetection_

// alias to use template instance with default allocator
using SimpleDetection =
  robot_interfaces::msg::SimpleDetection_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace robot_interfaces

#endif  // ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION__STRUCT_HPP_
