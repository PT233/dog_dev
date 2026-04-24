// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from robot_interfaces:srv/CalibrateCenter.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/srv/calibrate_center.hpp"


#ifndef ROBOT_INTERFACES__SRV__DETAIL__CALIBRATE_CENTER__STRUCT_HPP_
#define ROBOT_INTERFACES__SRV__DETAIL__CALIBRATE_CENTER__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__robot_interfaces__srv__CalibrateCenter_Request __attribute__((deprecated))
#else
# define DEPRECATED__robot_interfaces__srv__CalibrateCenter_Request __declspec(deprecated)
#endif

namespace robot_interfaces
{

namespace srv
{

// message struct
template<class ContainerAllocator>
struct CalibrateCenter_Request_
{
  using Type = CalibrateCenter_Request_<ContainerAllocator>;

  explicit CalibrateCenter_Request_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->center_x = 0l;
      this->center_y = 0l;
    }
  }

  explicit CalibrateCenter_Request_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->center_x = 0l;
      this->center_y = 0l;
    }
  }

  // field types and members
  using _center_x_type =
    int32_t;
  _center_x_type center_x;
  using _center_y_type =
    int32_t;
  _center_y_type center_y;

  // setters for named parameter idiom
  Type & set__center_x(
    const int32_t & _arg)
  {
    this->center_x = _arg;
    return *this;
  }
  Type & set__center_y(
    const int32_t & _arg)
  {
    this->center_y = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator> *;
  using ConstRawPtr =
    const robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__robot_interfaces__srv__CalibrateCenter_Request
    std::shared_ptr<robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__robot_interfaces__srv__CalibrateCenter_Request
    std::shared_ptr<robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const CalibrateCenter_Request_ & other) const
  {
    if (this->center_x != other.center_x) {
      return false;
    }
    if (this->center_y != other.center_y) {
      return false;
    }
    return true;
  }
  bool operator!=(const CalibrateCenter_Request_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct CalibrateCenter_Request_

// alias to use template instance with default allocator
using CalibrateCenter_Request =
  robot_interfaces::srv::CalibrateCenter_Request_<std::allocator<void>>;

// constant definitions

}  // namespace srv

}  // namespace robot_interfaces


#ifndef _WIN32
# define DEPRECATED__robot_interfaces__srv__CalibrateCenter_Response __attribute__((deprecated))
#else
# define DEPRECATED__robot_interfaces__srv__CalibrateCenter_Response __declspec(deprecated)
#endif

namespace robot_interfaces
{

namespace srv
{

// message struct
template<class ContainerAllocator>
struct CalibrateCenter_Response_
{
  using Type = CalibrateCenter_Response_<ContainerAllocator>;

  explicit CalibrateCenter_Response_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->success = false;
      this->message = "";
    }
  }

  explicit CalibrateCenter_Response_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : message(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->success = false;
      this->message = "";
    }
  }

  // field types and members
  using _success_type =
    bool;
  _success_type success;
  using _message_type =
    std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>>;
  _message_type message;

  // setters for named parameter idiom
  Type & set__success(
    const bool & _arg)
  {
    this->success = _arg;
    return *this;
  }
  Type & set__message(
    const std::basic_string<char, std::char_traits<char>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<char>> & _arg)
  {
    this->message = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator> *;
  using ConstRawPtr =
    const robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__robot_interfaces__srv__CalibrateCenter_Response
    std::shared_ptr<robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__robot_interfaces__srv__CalibrateCenter_Response
    std::shared_ptr<robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const CalibrateCenter_Response_ & other) const
  {
    if (this->success != other.success) {
      return false;
    }
    if (this->message != other.message) {
      return false;
    }
    return true;
  }
  bool operator!=(const CalibrateCenter_Response_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct CalibrateCenter_Response_

// alias to use template instance with default allocator
using CalibrateCenter_Response =
  robot_interfaces::srv::CalibrateCenter_Response_<std::allocator<void>>;

// constant definitions

}  // namespace srv

}  // namespace robot_interfaces


// Include directives for member types
// Member 'info'
#include "service_msgs/msg/detail/service_event_info__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__robot_interfaces__srv__CalibrateCenter_Event __attribute__((deprecated))
#else
# define DEPRECATED__robot_interfaces__srv__CalibrateCenter_Event __declspec(deprecated)
#endif

namespace robot_interfaces
{

namespace srv
{

// message struct
template<class ContainerAllocator>
struct CalibrateCenter_Event_
{
  using Type = CalibrateCenter_Event_<ContainerAllocator>;

  explicit CalibrateCenter_Event_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : info(_init)
  {
    (void)_init;
  }

  explicit CalibrateCenter_Event_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : info(_alloc, _init)
  {
    (void)_init;
  }

  // field types and members
  using _info_type =
    service_msgs::msg::ServiceEventInfo_<ContainerAllocator>;
  _info_type info;
  using _request_type =
    rosidl_runtime_cpp::BoundedVector<robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator>, 1, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator>>>;
  _request_type request;
  using _response_type =
    rosidl_runtime_cpp::BoundedVector<robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator>, 1, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator>>>;
  _response_type response;

  // setters for named parameter idiom
  Type & set__info(
    const service_msgs::msg::ServiceEventInfo_<ContainerAllocator> & _arg)
  {
    this->info = _arg;
    return *this;
  }
  Type & set__request(
    const rosidl_runtime_cpp::BoundedVector<robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator>, 1, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<robot_interfaces::srv::CalibrateCenter_Request_<ContainerAllocator>>> & _arg)
  {
    this->request = _arg;
    return *this;
  }
  Type & set__response(
    const rosidl_runtime_cpp::BoundedVector<robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator>, 1, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<robot_interfaces::srv::CalibrateCenter_Response_<ContainerAllocator>>> & _arg)
  {
    this->response = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    robot_interfaces::srv::CalibrateCenter_Event_<ContainerAllocator> *;
  using ConstRawPtr =
    const robot_interfaces::srv::CalibrateCenter_Event_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<robot_interfaces::srv::CalibrateCenter_Event_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<robot_interfaces::srv::CalibrateCenter_Event_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      robot_interfaces::srv::CalibrateCenter_Event_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<robot_interfaces::srv::CalibrateCenter_Event_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      robot_interfaces::srv::CalibrateCenter_Event_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<robot_interfaces::srv::CalibrateCenter_Event_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<robot_interfaces::srv::CalibrateCenter_Event_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<robot_interfaces::srv::CalibrateCenter_Event_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__robot_interfaces__srv__CalibrateCenter_Event
    std::shared_ptr<robot_interfaces::srv::CalibrateCenter_Event_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__robot_interfaces__srv__CalibrateCenter_Event
    std::shared_ptr<robot_interfaces::srv::CalibrateCenter_Event_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const CalibrateCenter_Event_ & other) const
  {
    if (this->info != other.info) {
      return false;
    }
    if (this->request != other.request) {
      return false;
    }
    if (this->response != other.response) {
      return false;
    }
    return true;
  }
  bool operator!=(const CalibrateCenter_Event_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct CalibrateCenter_Event_

// alias to use template instance with default allocator
using CalibrateCenter_Event =
  robot_interfaces::srv::CalibrateCenter_Event_<std::allocator<void>>;

// constant definitions

}  // namespace srv

}  // namespace robot_interfaces

namespace robot_interfaces
{

namespace srv
{

struct CalibrateCenter
{
  using Request = robot_interfaces::srv::CalibrateCenter_Request;
  using Response = robot_interfaces::srv::CalibrateCenter_Response;
  using Event = robot_interfaces::srv::CalibrateCenter_Event;
};

}  // namespace srv

}  // namespace robot_interfaces

#endif  // ROBOT_INTERFACES__SRV__DETAIL__CALIBRATE_CENTER__STRUCT_HPP_
