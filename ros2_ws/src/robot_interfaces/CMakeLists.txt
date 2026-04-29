cmake_minimum_required(VERSION 3.8)
project(robot_interfaces)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)

# 统一生成视觉检测、目标选择和标定服务的 ROS 接口代码。
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/TargetInfo.msg"
  "msg/Detection2D.msg"
  "msg/Detection2DArray.msg"
  "srv/SetTargetClass.srv"
  "srv/CalibrateCenter.srv"
  DEPENDENCIES std_msgs
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
