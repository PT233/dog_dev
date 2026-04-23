// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from robot_interfaces:msg/TargetInfo.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/msg/target_info.hpp"


#ifndef ROBOT_INTERFACES__MSG__DETAIL__TARGET_INFO__STRUCT_HPP_
#define ROBOT_INTERFACES__MSG__DETAIL__TARGET_INFO__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__robot_interfaces__msg__TargetInfo __attribute__((deprecated))
#else
# define DEPRECATED__robot_interfaces__msg__TargetInfo __declspec(deprecated)
#endif

namespace robot_interfaces
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct TargetInfo_
{
  using Type = TargetInfo_<ContainerAllocator>;

  explicit TargetInfo_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : header(_init)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->class_name = "";
      this->track_id = "";
      this->confidence = 0.0f;
      this->bbox_cx = 0l;
      this->bbox_cy = 0l;
      this->bbox_w = 0l;
      this->bbox_h = 0l;
      this->locked = false;
    }
  }

  explicit TargetInfo_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : header(_alloc, _init),
    class_name(_alloc),
    track_id(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->class_name = "";
      this->track_id = "";
      this->confidence = 0.0f;
      this->bbox_cx = 0l;
      this->bbox_cy = 0l;
      this->bbox_w = 0l;
      this->bbox_h = 0l;
      this->locked = false;
    }
  }

  // field types and members
  using _header_type =
    std_msgs::msg::Header_<ContainerAllocator>;
  _header_type header;
  using _class_name_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _class_name_type class_name;
  using _track_id_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _track_id_type track_id;
  using _confidence_type =
    float;
  _confidence_type confidence;
  using _bbox_cx_type =
    int32_t;
  _bbox_cx_type bbox_cx;
  using _bbox_cy_type =
    int32_t;
  _bbox_cy_type bbox_cy;
  using _bbox_w_type =
    int32_t;
  _bbox_w_type bbox_w;
  using _bbox_h_type =
    int32_t;
  _bbox_h_type bbox_h;
  using _locked_type =
    bool;
  _locked_type locked;

  // setters for named parameter idiom
  Type & set__header(
    const std_msgs::msg::Header_<ContainerAllocator> & _arg)
  {
    this->header = _arg;
    return *this;
  }
  Type & set__class_name(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->class_name = _arg;
    return *this;
  }
  Type & set__track_id(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->track_id = _arg;
    return *this;
  }
  Type & set__confidence(
    const float & _arg)
  {
    this->confidence = _arg;
    return *this;
  }
  Type & set__bbox_cx(
    const int32_t & _arg)
  {
    this->bbox_cx = _arg;
    return *this;
  }
  Type & set__bbox_cy(
    const int32_t & _arg)
  {
    this->bbox_cy = _arg;
    return *this;
  }
  Type & set__bbox_w(
    const int32_t & _arg)
  {
    this->bbox_w = _arg;
    return *this;
  }
  Type & set__bbox_h(
    const int32_t & _arg)
  {
    this->bbox_h = _arg;
    return *this;
  }
  Type & set__locked(
    const bool & _arg)
  {
    this->locked = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    robot_interfaces::msg::TargetInfo_<ContainerAllocator> *;
  using ConstRawPtr =
    const robot_interfaces::msg::TargetInfo_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<robot_interfaces::msg::TargetInfo_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<robot_interfaces::msg::TargetInfo_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      robot_interfaces::msg::TargetInfo_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<robot_interfaces::msg::TargetInfo_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      robot_interfaces::msg::TargetInfo_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<robot_interfaces::msg::TargetInfo_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<robot_interfaces::msg::TargetInfo_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<robot_interfaces::msg::TargetInfo_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__robot_interfaces__msg__TargetInfo
    std::shared_ptr<robot_interfaces::msg::TargetInfo_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__robot_interfaces__msg__TargetInfo
    std::shared_ptr<robot_interfaces::msg::TargetInfo_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const TargetInfo_ & other) const
  {
    if (this->header != other.header) {
      return false;
    }
    if (this->class_name != other.class_name) {
      return false;
    }
    if (this->track_id != other.track_id) {
      return false;
    }
    if (this->confidence != other.confidence) {
      return false;
    }
    if (this->bbox_cx != other.bbox_cx) {
      return false;
    }
    if (this->bbox_cy != other.bbox_cy) {
      return false;
    }
    if (this->bbox_w != other.bbox_w) {
      return false;
    }
    if (this->bbox_h != other.bbox_h) {
      return false;
    }
    if (this->locked != other.locked) {
      return false;
    }
    return true;
  }
  bool operator!=(const TargetInfo_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct TargetInfo_

// alias to use template instance with default allocator
using TargetInfo =
  robot_interfaces::msg::TargetInfo_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace robot_interfaces

#endif  // ROBOT_INTERFACES__MSG__DETAIL__TARGET_INFO__STRUCT_HPP_
