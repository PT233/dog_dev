cmake_minimum_required(VERSION 3.8)
project(uart_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

# Build executable
add_executable(uart_bridge_node
  src/uart_bridge_node.cpp
  src/frame_parser.cpp
  src/frame_encoder.cpp
  src/uart_protocol.c
)
target_include_directories(uart_bridge_node PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
ament_target_dependencies(uart_bridge_node rclcpp sensor_msgs)

install(TARGETS uart_bridge_node DESTINATION lib/${PROJECT_NAME})

# Install config and launch files
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

ament_package()
