// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from robot_interfaces:msg/SimpleDetection.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/msg/simple_detection.hpp"


#ifndef ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION__BUILDER_HPP_
#define ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "robot_interfaces/msg/detail/simple_detection__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace robot_interfaces
{

namespace msg
{

namespace builder
{

class Init_SimpleDetection_track_id
{
public:
  explicit Init_SimpleDetection_track_id(::robot_interfaces::msg::SimpleDetection & msg)
  : msg_(msg)
  {}
  ::robot_interfaces::msg::SimpleDetection track_id(::robot_interfaces::msg::SimpleDetection::_track_id_type arg)
  {
    msg_.track_id = std::move(arg);
    return std::move(msg_);
  }

private:
  ::robot_interfaces::msg::SimpleDetection msg_;
};

class Init_SimpleDetection_class_id
{
public:
  explicit Init_SimpleDetection_class_id(::robot_interfaces::msg::SimpleDetection & msg)
  : msg_(msg)
  {}
  Init_SimpleDetection_track_id class_id(::robot_interfaces::msg::SimpleDetection::_class_id_type arg)
  {
    msg_.class_id = std::move(arg);
    return Init_SimpleDetection_track_id(msg_);
  }

private:
  ::robot_interfaces::msg::SimpleDetection msg_;
};

class Init_SimpleDetection_confidence
{
public:
  explicit Init_SimpleDetection_confidence(::robot_interfaces::msg::SimpleDetection & msg)
  : msg_(msg)
  {}
  Init_SimpleDetection_class_id confidence(::robot_interfaces::msg::SimpleDetection::_confidence_type arg)
  {
    msg_.confidence = std::move(arg);
    return Init_SimpleDetection_class_id(msg_);
  }

private:
  ::robot_interfaces::msg::SimpleDetection msg_;
};

class Init_SimpleDetection_height
{
public:
  explicit Init_SimpleDetection_height(::robot_interfaces::msg::SimpleDetection & msg)
  : msg_(msg)
  {}
  Init_SimpleDetection_confidence height(::robot_interfaces::msg::SimpleDetection::_height_type arg)
  {
    msg_.height = std::move(arg);
    return Init_SimpleDetection_confidence(msg_);
  }

private:
  ::robot_interfaces::msg::SimpleDetection msg_;
};

class Init_SimpleDetection_width
{
public:
  explicit Init_SimpleDetection_width(::robot_interfaces::msg::SimpleDetection & msg)
  : msg_(msg)
  {}
  Init_SimpleDetection_height width(::robot_interfaces::msg::SimpleDetection::_width_type arg)
  {
    msg_.width = std::move(arg);
    return Init_SimpleDetection_height(msg_);
  }

private:
  ::robot_interfaces::msg::SimpleDetection msg_;
};

class Init_SimpleDetection_center_y
{
public:
  explicit Init_SimpleDetection_center_y(::robot_interfaces::msg::SimpleDetection & msg)
  : msg_(msg)
  {}
  Init_SimpleDetection_width center_y(::robot_interfaces::msg::SimpleDetection::_center_y_type arg)
  {
    msg_.center_y = std::move(arg);
    return Init_SimpleDetection_width(msg_);
  }

private:
  ::robot_interfaces::msg::SimpleDetection msg_;
};

class Init_SimpleDetection_center_x
{
public:
  Init_SimpleDetection_center_x()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SimpleDetection_center_y center_x(::robot_interfaces::msg::SimpleDetection::_center_x_type arg)
  {
    msg_.center_x = std::move(arg);
    return Init_SimpleDetection_center_y(msg_);
  }

private:
  ::robot_interfaces::msg::SimpleDetection msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::robot_interfaces::msg::SimpleDetection>()
{
  return robot_interfaces::msg::builder::Init_SimpleDetection_center_x();
}

}  // namespace robot_interfaces

#endif  // ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION__BUILDER_HPP_
