#pragma once

#include "rclcpp/rclcpp.hpp"

#include <memory>
#include <type_traits>
#include <utility>

namespace project_shared {

template <typename NodeT, typename... Args>
int spin_single_node_main(int argc, char** argv, Args&&... args) {
  static_assert(std::is_base_of_v<rclcpp::Node, NodeT>,
                "spin_single_node_main requires an rclcpp::Node type");
  rclcpp::init(argc, argv);
  auto node = std::make_shared<NodeT>(std::forward<Args>(args)...);
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}

}  // namespace project_shared
