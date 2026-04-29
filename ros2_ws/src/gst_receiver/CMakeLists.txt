cmake_minimum_required(VERSION 3.8)
project(gst_receiver)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 依赖：GStreamer 接收/解码 UDP H.264，OpenCV 用于帧格式视图。
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)

# 接收节点主体编成库，供 robot_bringup 的 vision_front 同进程链接。
add_library(gst_receiver_lib src/gst_receiver_node.cpp)
target_include_directories(gst_receiver_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${GST_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(gst_receiver_lib ${GST_LIBRARIES} ${OpenCV_LIBS})
ament_target_dependencies(gst_receiver_lib rclcpp sensor_msgs cv_bridge)

# 单独运行的 GStreamer 接收节点。
add_executable(gst_receiver_node src/gst_receiver_main.cpp)
target_include_directories(gst_receiver_node PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
target_link_libraries(gst_receiver_node gst_receiver_lib)
ament_target_dependencies(gst_receiver_node rclcpp sensor_msgs cv_bridge)

# 安装头文件，供其他包链接库目标。
install(DIRECTORY include/ DESTINATION include)

# 安装启动文件。
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

# 安装可执行文件和库。
install(TARGETS gst_receiver_node
  DESTINATION lib/${PROJECT_NAME})
install(TARGETS gst_receiver_lib
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

ament_export_libraries(gst_receiver_lib)
ament_export_include_directories(include)
ament_package()
