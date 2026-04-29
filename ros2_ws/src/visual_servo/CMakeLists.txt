cmake_minimum_required(VERSION 3.8)
project(visual_servo)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 依赖：geometry_msgs 接收像素误差，sensor_msgs 发布/订阅 JointState。
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)

# 视觉伺服节点：PID + 简化步态控制，输出私有 servo_command。
add_executable(visual_servo_node_exe
  src/leg_motion_controller_node.cpp
  src/pid_controller.cpp
  src/main.cpp)
target_include_directories(visual_servo_node_exe PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_include_directories(visual_servo_node_exe PRIVATE
  # 访问仓库根 shared/servo_names.hpp，保证舵机名顺序全项目一致。
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
ament_target_dependencies(visual_servo_node_exe rclcpp geometry_msgs sensor_msgs)

install(TARGETS visual_servo_node_exe DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})
# visual_servo.yaml 位于仓库根 config/，安装后由 launch 加载。
install(FILES ../../../config/visual_servo.yaml DESTINATION share/${PROJECT_NAME}/config)

if(BUILD_TESTING)
  enable_testing()
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()

  add_executable(test_pid_controller ../../../tests/test_pid_controller.cpp src/pid_controller.cpp)
  target_include_directories(test_pid_controller PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
  add_test(NAME test_pid_controller COMMAND test_pid_controller)
endif()

ament_package()
