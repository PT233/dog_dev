cmake_minimum_required(VERSION 3.8)
project(stereo_splitter)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 依赖：接收 sensor_msgs/Image，OpenCV 目前主要用于图像处理扩展。
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)

# 裁剪节点主体编成库，供 vision_front 同进程链接。
add_library(stereo_splitter_lib src/stereo_splitter_node.cpp)
target_include_directories(stereo_splitter_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${OpenCV_INCLUDE_DIRS})
target_link_libraries(stereo_splitter_lib ${OpenCV_LIBS})
ament_target_dependencies(stereo_splitter_lib rclcpp sensor_msgs cv_bridge)

# 单独运行的裁剪节点。
add_executable(stereo_splitter_node src/stereo_splitter_main.cpp)
target_include_directories(stereo_splitter_node PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
target_link_libraries(stereo_splitter_node stereo_splitter_lib)
ament_target_dependencies(stereo_splitter_node rclcpp sensor_msgs cv_bridge)

# 安装头文件，供其他包链接库目标。
install(DIRECTORY include/ DESTINATION include)

# 安装启动文件。
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

# 安装可执行文件和库。
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
