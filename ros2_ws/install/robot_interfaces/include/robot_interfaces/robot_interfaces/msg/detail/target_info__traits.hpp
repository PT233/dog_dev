// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from robot_interfaces:msg/TargetInfo.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "robot_interfaces/msg/target_info.hpp"


#ifndef ROBOT_INTERFACES__MSG__DETAIL__TARGET_INFO__TRAITS_HPP_
#define ROBOT_INTERFACES__MSG__DETAIL__TARGET_INFO__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "robot_interfaces/msg/detail/target_info__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__traits.hpp"

namespace robot_interfaces
{

namespace msg
{

inline void to_flow_style_yaml(
  const TargetInfo & msg,
  std::ostream & out)
{
  out << "{";
  // member: header
  {
    out << "header: ";
    to_flow_style_yaml(msg.header, out);
    out << ", ";
  }

  // member: class_name
  {
    out << "class_name: ";
    rosidl_generator_traits::value_to_yaml(msg.class_name, out);
    out << ", ";
  }

  // member: track_id
  {
    out << "track_id: ";
    rosidl_generator_traits::value_to_yaml(msg.track_id, out);
    out << ", ";
  }

  // member: confidence
  {
    out << "confidence: ";
    rosidl_generator_traits::value_to_yaml(msg.confidence, out);
    out << ", ";
  }

  // member: bbox_cx
  {
    out << "bbox_cx: ";
    rosidl_generator_traits::value_to_yaml(msg.bbox_cx, out);
    out << ", ";
  }

  // member: bbox_cy
  {
    out << "bbox_cy: ";
    rosidl_generator_traits::value_to_yaml(msg.bbox_cy, out);
    out << ", ";
  }

  // member: bbox_w
  {
    out << "bbox_w: ";
    rosidl_generator_traits::value_to_yaml(msg.bbox_w, out);
    out << ", ";
  }

  // member: bbox_h
  {
    out << "bbox_h: ";
    rosidl_generator_traits::value_to_yaml(msg.bbox_h, out);
    out << ", ";
  }

  // member: locked
  {
    out << "locked: ";
    rosidl_generator_traits::value_to_yaml(msg.locked, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const TargetInfo & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: header
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "header:\n";
    to_block_style_yaml(msg.header, out, indentation + 2);
  }

  // member: class_name
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "class_name: ";
    rosidl_generator_traits::value_to_yaml(msg.class_name, out);
    out << "\n";
  }

  // member: track_id
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "track_id: ";
    rosidl_generator_traits::value_to_yaml(msg.track_id, out);
    out << "\n";
  }

  // member: confidence
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "confidence: ";
    rosidl_generator_traits::value_to_yaml(msg.confidence, out);
    out << "\n";
  }

  // member: bbox_cx
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "bbox_cx: ";
    rosidl_generator_traits::value_to_yaml(msg.bbox_cx, out);
    out << "\n";
  }

  // member: bbox_cy
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "bbox_cy: ";
    rosidl_generator_traits::value_to_yaml(msg.bbox_cy, out);
    out << "\n";
  }

  // member: bbox_w
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "bbox_w: ";
    rosidl_generator_traits::value_to_yaml(msg.bbox_w, out);
    out << "\n";
  }

  // member: bbox_h
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "bbox_h: ";
    rosidl_generator_traits::value_to_yaml(msg.bbox_h, out);
    out << "\n";
  }

  // member: locked
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "locked: ";
    rosidl_generator_traits::value_to_yaml(msg.locked, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const TargetInfo & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace msg

}  // namespace robot_interfaces

namespace rosidl_generator_traits
{

[[deprecated("use robot_interfaces::msg::to_block_style_yaml() instead")]]
inline void to_yaml(
  const robot_interfaces::msg::TargetInfo & msg,
  std::ostream & out, size_t indentation = 0)
{
  robot_interfaces::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use robot_interfaces::msg::to_yaml() instead")]]
inline std::string to_yaml(const robot_interfaces::msg::TargetInfo & msg)
{
  return robot_interfaces::msg::to_yaml(msg);
}

template<>
inline const char * data_type<robot_interfaces::msg::TargetInfo>()
{
  return "robot_interfaces::msg::TargetInfo";
}

template<>
inline const char * name<robot_interfaces::msg::TargetInfo>()
{
  return "robot_interfaces/msg/TargetInfo";
}

template<>
struct has_fixed_size<robot_interfaces::msg::TargetInfo>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<robot_interfaces::msg::TargetInfo>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<robot_interfaces::msg::TargetInfo>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // ROBOT_INTERFACES__MSG__DETAIL__TARGET_INFO__TRAITS_HPP_
