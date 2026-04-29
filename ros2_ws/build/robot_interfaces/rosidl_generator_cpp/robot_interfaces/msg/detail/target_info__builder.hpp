// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from robot_interfaces:msg/TargetInfo.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/msg/target_info.hpp"


#ifndef ROBOT_INTERFACES__MSG__DETAIL__TARGET_INFO__BUILDER_HPP_
#define ROBOT_INTERFACES__MSG__DETAIL__TARGET_INFO__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "robot_interfaces/msg/detail/target_info__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace robot_interfaces
{

namespace msg
{

namespace builder
{

class Init_TargetInfo_is_locked
{
public:
  explicit Init_TargetInfo_is_locked(::robot_interfaces::msg::TargetInfo & msg)
  : msg_(msg)
  {}
  ::robot_interfaces::msg::TargetInfo is_locked(::robot_interfaces::msg::TargetInfo::_is_locked_type arg)
  {
    msg_.is_locked = std::move(arg);
    return std::move(msg_);
  }

private:
  ::robot_interfaces::msg::TargetInfo msg_;
};

class Init_TargetInfo_bounding_box_height
{
public:
  explicit Init_TargetInfo_bounding_box_height(::robot_interfaces::msg::TargetInfo & msg)
  : msg_(msg)
  {}
  Init_TargetInfo_is_locked bounding_box_height(::robot_interfaces::msg::TargetInfo::_bounding_box_height_type arg)
  {
    msg_.bounding_box_height = std::move(arg);
    return Init_TargetInfo_is_locked(msg_);
  }

private:
  ::robot_interfaces::msg::TargetInfo msg_;
};

class Init_TargetInfo_bounding_box_width
{
public:
  explicit Init_TargetInfo_bounding_box_width(::robot_interfaces::msg::TargetInfo & msg)
  : msg_(msg)
  {}
  Init_TargetInfo_bounding_box_height bounding_box_width(::robot_interfaces::msg::TargetInfo::_bounding_box_width_type arg)
  {
    msg_.bounding_box_width = std::move(arg);
    return Init_TargetInfo_bounding_box_height(msg_);
  }

private:
  ::robot_interfaces::msg::TargetInfo msg_;
};

class Init_TargetInfo_bounding_box_center_y
{
public:
  explicit Init_TargetInfo_bounding_box_center_y(::robot_interfaces::msg::TargetInfo & msg)
  : msg_(msg)
  {}
  Init_TargetInfo_bounding_box_width bounding_box_center_y(::robot_interfaces::msg::TargetInfo::_bounding_box_center_y_type arg)
  {
    msg_.bounding_box_center_y = std::move(arg);
    return Init_TargetInfo_bounding_box_width(msg_);
  }

private:
  ::robot_interfaces::msg::TargetInfo msg_;
};

class Init_TargetInfo_bounding_box_center_x
{
public:
  explicit Init_TargetInfo_bounding_box_center_x(::robot_interfaces::msg::TargetInfo & msg)
  : msg_(msg)
  {}
  Init_TargetInfo_bounding_box_center_y bounding_box_center_x(::robot_interfaces::msg::TargetInfo::_bounding_box_center_x_type arg)
  {
    msg_.bounding_box_center_x = std::move(arg);
    return Init_TargetInfo_bounding_box_center_y(msg_);
  }

private:
  ::robot_interfaces::msg::TargetInfo msg_;
};

class Init_TargetInfo_confidence
{
public:
  explicit Init_TargetInfo_confidence(::robot_interfaces::msg::TargetInfo & msg)
  : msg_(msg)
  {}
  Init_TargetInfo_bounding_box_center_x confidence(::robot_interfaces::msg::TargetInfo::_confidence_type arg)
  {
    msg_.confidence = std::move(arg);
    return Init_TargetInfo_bounding_box_center_x(msg_);
  }

private:
  ::robot_interfaces::msg::TargetInfo msg_;
};

class Init_TargetInfo_track_id
{
public:
  explicit Init_TargetInfo_track_id(::robot_interfaces::msg::TargetInfo & msg)
  : msg_(msg)
  {}
  Init_TargetInfo_confidence track_id(::robot_interfaces::msg::TargetInfo::_track_id_type arg)
  {
    msg_.track_id = std::move(arg);
    return Init_TargetInfo_confidence(msg_);
  }

private:
  ::robot_interfaces::msg::TargetInfo msg_;
};

class Init_TargetInfo_class_name
{
public:
  explicit Init_TargetInfo_class_name(::robot_interfaces::msg::TargetInfo & msg)
  : msg_(msg)
  {}
  Init_TargetInfo_track_id class_name(::robot_interfaces::msg::TargetInfo::_class_name_type arg)
  {
    msg_.class_name = std::move(arg);
    return Init_TargetInfo_track_id(msg_);
  }

private:
  ::robot_interfaces::msg::TargetInfo msg_;
};

class Init_TargetInfo_header
{
public:
  Init_TargetInfo_header()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_TargetInfo_class_name header(::robot_interfaces::msg::TargetInfo::_header_type arg)
  {
    msg_.header = std::move(arg);
    return Init_TargetInfo_class_name(msg_);
  }

private:
  ::robot_interfaces::msg::TargetInfo msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::robot_interfaces::msg::TargetInfo>()
{
  return robot_interfaces::msg::builder::Init_TargetInfo_header();
}

}  // namespace robot_interfaces

#endif  // ROBOT_INTERFACES__MSG__DETAIL__TARGET_INFO__BUILDER_HPP_
