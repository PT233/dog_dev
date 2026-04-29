cmake_minimum_required(VERSION 3.8)
project(tracker_node)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 依赖：robot_interfaces 提供检测数组消息，sensor_msgs 当前随视觉链保留。
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(robot_interfaces REQUIRED)

# ByteTracker 单独编成库，便于单元测试复用。
add_library(byte_tracker src/byte_tracker.cpp)
target_include_directories(byte_tracker PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

# tracker_node_exe 负责 ROS 消息转换和调用 ByteTracker。
add_executable(tracker_node_exe src/tracker_node.cpp src/main.cpp)
target_link_libraries(tracker_node_exe byte_tracker)
target_include_directories(tracker_node_exe PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
ament_target_dependencies(tracker_node_exe rclcpp sensor_msgs robot_interfaces)

install(TARGETS tracker_node_exe DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch config DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  find_package(GTest REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()

  # ByteTracker 单元测试，只验证跟踪算法本身。
  add_executable(test_byte_tracker test/test_byte_tracker.cpp)
  target_link_libraries(test_byte_tracker byte_tracker)
  target_include_directories(test_byte_tracker PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

  # 集成测试覆盖 ROS 消息到跟踪输出的基本链路。
  add_executable(test_tracker_node_integration test/test_tracker_node_integration.cpp)
  target_link_libraries(test_tracker_node_integration byte_tracker GTest::GTest GTest::Main)
  target_include_directories(test_tracker_node_integration PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

  add_test(NAME test_tracker_node_integration COMMAND test_tracker_node_integration)
endif()

ament_package()
