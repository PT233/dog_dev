cmake_minimum_required(VERSION 3.8)
project(behavior_node)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 依赖：rclcpp 提供节点基础，robot_interfaces 提供检测消息和服务，geometry_msgs 提供像素误差向量。
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(robot_interfaces REQUIRED)
find_package(geometry_msgs REQUIRED)

# 行为节点可执行文件：从跟踪结果中选择目标并发布 /pixel_error。
add_executable(behavior_node_exe src/behavior_node.cpp src/main.cpp)
target_include_directories(behavior_node_exe PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_include_directories(behavior_node_exe PRIVATE
  # 引入仓库根目录，供 shared/ 公共头文件使用。
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
ament_target_dependencies(behavior_node_exe rclcpp robot_interfaces geometry_msgs)

install(TARGETS behavior_node_exe DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})
# behavior.yaml 位于仓库根 config/，安装到包 share/config 后供 launch 使用。
install(FILES ../../../config/behavior.yaml DESTINATION share/${PROJECT_NAME}/config)

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

ament_package()
