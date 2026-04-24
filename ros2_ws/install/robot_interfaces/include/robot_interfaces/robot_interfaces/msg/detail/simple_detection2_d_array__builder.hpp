// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from robot_interfaces:msg/SimpleDetection2DArray.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/msg/simple_detection2_d_array.hpp"


#ifndef ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION2_D_ARRAY__BUILDER_HPP_
#define ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION2_D_ARRAY__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "robot_interfaces/msg/detail/simple_detection2_d_array__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace robot_interfaces
{

namespace msg
{

namespace builder
{

class Init_SimpleDetection2DArray_detections
{
public:
  explicit Init_SimpleDetection2DArray_detections(::robot_interfaces::msg::SimpleDetection2DArray & msg)
  : msg_(msg)
  {}
  ::robot_interfaces::msg::SimpleDetection2DArray detections(::robot_interfaces::msg::SimpleDetection2DArray::_detections_type arg)
  {
    msg_.detections = std::move(arg);
    return std::move(msg_);
  }

private:
  ::robot_interfaces::msg::SimpleDetection2DArray msg_;
};

class Init_SimpleDetection2DArray_header
{
public:
  Init_SimpleDetection2DArray_header()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_SimpleDetection2DArray_detections header(::robot_interfaces::msg::SimpleDetection2DArray::_header_type arg)
  {
    msg_.header = std::move(arg);
    return Init_SimpleDetection2DArray_detections(msg_);
  }

private:
  ::robot_interfaces::msg::SimpleDetection2DArray msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::robot_interfaces::msg::SimpleDetection2DArray>()
{
  return robot_interfaces::msg::builder::Init_SimpleDetection2DArray_header();
}

}  // namespace robot_interfaces

#endif  // ROBOT_INTERFACES__MSG__DETAIL__SIMPLE_DETECTION2_D_ARRAY__BUILDER_HPP_
