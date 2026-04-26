cmake_minimum_required(VERSION 3.8)
project(stereo_splitter)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)

# Library target (for intra-process use and linking)
add_library(stereo_splitter_lib src/stereo_splitter_node.cpp)
target_include_directories(stereo_splitter_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${OpenCV_INCLUDE_DIRS})
target_link_libraries(stereo_splitter_lib ${OpenCV_LIBS})
ament_target_dependencies(stereo_splitter_lib rclcpp sensor_msgs cv_bridge)

# Executable target
add_executable(stereo_splitter_node src/stereo_splitter_main.cpp)
target_link_libraries(stereo_splitter_node stereo_splitter_lib)
ament_target_dependencies(stereo_splitter_node rclcpp sensor_msgs cv_bridge)

# Install headers
install(DIRECTORY include/ DESTINATION include)

# Install launch files
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

# Install targets (executable and library)
install(TARGETS stereo_splitter_node
  DESTINATION lib/${PROJECT_NAME})
install(TARGETS stereo_splitter_lib
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_libraries(stereo_splitter_lib)
ament_export_include_directories(include)
ament_package()
