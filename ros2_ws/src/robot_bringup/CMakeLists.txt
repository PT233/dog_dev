cmake_minimum_required(VERSION 3.8)
project(robot_bringup)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

# Install Python scripts as executables
install(PROGRAMS
  ../../../scripts/mock_uart_bridge.py
  DESTINATION lib/${PROJECT_NAME}
  RENAME mock_uart_bridge
)

ament_package()
