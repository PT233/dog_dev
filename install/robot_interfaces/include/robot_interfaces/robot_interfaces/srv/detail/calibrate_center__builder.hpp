// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from robot_interfaces:srv/CalibrateCenter.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/srv/calibrate_center.hpp"


#ifndef ROBOT_INTERFACES__SRV__DETAIL__CALIBRATE_CENTER__BUILDER_HPP_
#define ROBOT_INTERFACES__SRV__DETAIL__CALIBRATE_CENTER__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "robot_interfaces/srv/detail/calibrate_center__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace robot_interfaces
{

namespace srv
{

namespace builder
{

class Init_CalibrateCenter_Request_center_y
{
public:
  explicit Init_CalibrateCenter_Request_center_y(::robot_interfaces::srv::CalibrateCenter_Request & msg)
  : msg_(msg)
  {}
  ::robot_interfaces::srv::CalibrateCenter_Request center_y(::robot_interfaces::srv::CalibrateCenter_Request::_center_y_type arg)
  {
    msg_.center_y = std::move(arg);
    return std::move(msg_);
  }

private:
  ::robot_interfaces::srv::CalibrateCenter_Request msg_;
};

class Init_CalibrateCenter_Request_center_x
{
public:
  Init_CalibrateCenter_Request_center_x()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_CalibrateCenter_Request_center_y center_x(::robot_interfaces::srv::CalibrateCenter_Request::_center_x_type arg)
  {
    msg_.center_x = std::move(arg);
    return Init_CalibrateCenter_Request_center_y(msg_);
  }

private:
  ::robot_interfaces::srv::CalibrateCenter_Request msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::robot_interfaces::srv::CalibrateCenter_Request>()
{
  return robot_interfaces::srv::builder::Init_CalibrateCenter_Request_center_x();
}

}  // namespace robot_interfaces


namespace robot_interfaces
{

namespace srv
{

namespace builder
{

class Init_CalibrateCenter_Response_message
{
public:
  explicit Init_CalibrateCenter_Response_message(::robot_interfaces::srv::CalibrateCenter_Response & msg)
  : msg_(msg)
  {}
  ::robot_interfaces::srv::CalibrateCenter_Response message(::robot_interfaces::srv::CalibrateCenter_Response::_message_type arg)
  {
    msg_.message = std::move(arg);
    return std::move(msg_);
  }

private:
  ::robot_interfaces::srv::CalibrateCenter_Response msg_;
};

class Init_CalibrateCenter_Response_success
{
public:
  Init_CalibrateCenter_Response_success()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_CalibrateCenter_Response_message success(::robot_interfaces::srv::CalibrateCenter_Response::_success_type arg)
  {
    msg_.success = std::move(arg);
    return Init_CalibrateCenter_Response_message(msg_);
  }

private:
  ::robot_interfaces::srv::CalibrateCenter_Response msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::robot_interfaces::srv::CalibrateCenter_Response>()
{
  return robot_interfaces::srv::builder::Init_CalibrateCenter_Response_success();
}

}  // namespace robot_interfaces


namespace robot_interfaces
{

namespace srv
{

namespace builder
{

class Init_CalibrateCenter_Event_response
{
public:
  explicit Init_CalibrateCenter_Event_response(::robot_interfaces::srv::CalibrateCenter_Event & msg)
  : msg_(msg)
  {}
  ::robot_interfaces::srv::CalibrateCenter_Event response(::robot_interfaces::srv::CalibrateCenter_Event::_response_type arg)
  {
    msg_.response = std::move(arg);
    return std::move(msg_);
  }

private:
  ::robot_interfaces::srv::CalibrateCenter_Event msg_;
};

class Init_CalibrateCenter_Event_request
{
public:
  explicit Init_CalibrateCenter_Event_request(::robot_interfaces::srv::CalibrateCenter_Event & msg)
  : msg_(msg)
  {}
  Init_CalibrateCenter_Event_response request(::robot_interfaces::srv::CalibrateCenter_Event::_request_type arg)
  {
    msg_.request = std::move(arg);
    return Init_CalibrateCenter_Event_response(msg_);
  }

private:
  ::robot_interfaces::srv::CalibrateCenter_Event msg_;
};

class Init_CalibrateCenter_Event_info
{
public:
  Init_CalibrateCenter_Event_info()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_CalibrateCenter_Event_request info(::robot_interfaces::srv::CalibrateCenter_Event::_info_type arg)
  {
    msg_.info = std::move(arg);
    return Init_CalibrateCenter_Event_request(msg_);
  }

private:
  ::robot_interfaces::srv::CalibrateCenter_Event msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::robot_interfaces::srv::CalibrateCenter_Event>()
{
  return robot_interfaces::srv::builder::Init_CalibrateCenter_Event_info();
}

}  // namespace robot_interfaces

#endif  // ROBOT_INTERFACES__SRV__DETAIL__CALIBRATE_CENTER__BUILDER_HPP_
