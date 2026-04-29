cmake_minimum_required(VERSION 3.8)
project(uart_bridge)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 依赖：rclcpp 提供 ROS 节点，sensor_msgs 提供舵机命令/状态的 JointState。
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

# UART 桥接可执行文件由传输、协议处理、时间同步、帧编解码和 CRC 实现组成。
add_executable(uart_bridge_node
  src/uart_bridge_time_sync.cpp
  src/uart_bridge_transport.cpp
  src/uart_bridge_protocol.cpp
  src/uart_bridge_main.cpp
  src/frame_parser.cpp
  src/frame_encoder.cpp
  src/uart_protocol.c
)
target_include_directories(uart_bridge_node PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# 访问仓库根 shared/servo_names.hpp，保证 ROS 关节名与 STM32 servo_id 一致。
target_include_directories(uart_bridge_node PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
ament_target_dependencies(uart_bridge_node rclcpp sensor_msgs)

install(TARGETS uart_bridge_node DESTINATION lib/${PROJECT_NAME})

# 安装串口参数和启动文件。
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

ament_package()
