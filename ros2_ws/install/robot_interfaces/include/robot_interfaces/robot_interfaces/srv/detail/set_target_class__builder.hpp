// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from robot_interfaces:srv/SetTargetClass.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/srv/set_target_class.hpp"


#ifndef ROBOT_INTERFACES__SRV__DETAIL__SET_TARGET_CLASS__BUILDER_HPP_
#define ROBOT_INTERFACES__SRV__DETAIL__SET_TARGET_CLASS__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "robot_interfaces/srv/detail/set_target_class__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace robot_interfaces
{

namespace srv
{

namespace builder
{

class Init_SetTargetClass_Request_class_name
{
public:
  Init_SetTargetClass_Request_class_name()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::robot_interfaces::srv::SetTargetClass_Request class_name(::robot_interfaces::srv::SetTargetClass_Request::_class_name_type arg)
  {
    msg_.class_name = std::move(arg);
    return std::move(msg_);
  }

private:
  ::robot_interfaces::srv::SetTargetClass_Request msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::robot_interfaces::srv::SetTargetClass_Request>()
{
  return robot_interfaces::srv::builder::Init_SetTargetClass_Request_class_name();
}

}  // namespace robot_interfaces


namespace robot_interfaces
{

namespace srv
{

namespace builder
{

class Init_SetTargetClass_Response_message
{
public:
  explicit Init_SetTargetClass_Response_message(::robot_interfaces::srv::SetTargetClass_Response & msg)
  : msg_(msg)
  {}
  ::robot_interfaces::srv::SetTargetClass_Response message(::robot_interfaces::srv::SetTargetClass_Response::_message_type arg)
  {
    msg_.message = std::move(arg);
    return std::move(msg_);
  }

private:
  ::robot_interfaces::srv::SetTargetClass_Response msg_;
};

class Init_SetTargetClass_Response_success
{
public:
  Init_SetTargetClass_Response_success()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SetTargetClass_Response_message success(::robot_interfaces::srv::SetTargetClass_Response::_success_type arg)
  {
    msg_.success = std::move(arg);
    return Init_SetTargetClass_Response_message(msg_);
  }

private:
  ::robot_interfaces::srv::SetTargetClass_Response msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::robot_interfaces::srv::SetTargetClass_Response>()
{
  return robot_interfaces::srv::builder::Init_SetTargetClass_Response_success();
}

}  // namespace robot_interfaces


namespace robot_interfaces
{

namespace srv
{

namespace builder
{

class Init_SetTargetClass_Event_response
{
public:
  explicit Init_SetTargetClass_Event_response(::robot_interfaces::srv::SetTargetClass_Event & msg)
  : msg_(msg)
  {}
  ::robot_interfaces::srv::SetTargetClass_Event response(::robot_interfaces::srv::SetTargetClass_Event::_response_type arg)
  {
    msg_.response = std::move(arg);
    return std::move(msg_);
  }

private:
  ::robot_interfaces::srv::SetTargetClass_Event msg_;
};

class Init_SetTargetClass_Event_request
{
public:
  explicit Init_SetTargetClass_Event_request(::robot_interfaces::srv::SetTargetClass_Event & msg)
  : msg_(msg)
  {}
  Init_SetTargetClass_Event_response request(::robot_interfaces::srv::SetTargetClass_Event::_request_type arg)
  {
    msg_.request = std::move(arg);
    return Init_SetTargetClass_Event_response(msg_);
  }

private:
  ::robot_interfaces::srv::SetTargetClass_Event msg_;
};

class Init_SetTargetClass_Event_info
{
public:
  Init_SetTargetClass_Event_info()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SetTargetClass_Event_request info(::robot_interfaces::srv::SetTargetClass_Event::_info_type arg)
  {
    msg_.info = std::move(arg);
    return Init_SetTargetClass_Event_request(msg_);
  }

private:
  ::robot_interfaces::srv::SetTargetClass_Event msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::robot_interfaces::srv::SetTargetClass_Event>()
{
  return robot_interfaces::srv::builder::Init_SetTargetClass_Event_info();
}

}  // namespace robot_interfaces

#endif  // ROBOT_INTERFACES__SRV__DETAIL__SET_TARGET_CLASS__BUILDER_HPP_
