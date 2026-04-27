cmake_minimum_required(VERSION 3.8)
project(robot_bringup)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

option(BUILD_VISION_FRONT "Build the WSL2 intra-process vision_front executable" OFF)

if(BUILD_VISION_FRONT)
  find_package(rclcpp REQUIRED)
  find_package(sensor_msgs REQUIRED)
  find_package(robot_interfaces REQUIRED)
  find_package(OpenCV REQUIRED)
  find_package(PkgConfig REQUIRED)
  find_package(gst_receiver REQUIRED)
  find_package(stereo_splitter REQUIRED)
  find_package(detection_node REQUIRED)

  pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)

  set(ONNXRUNTIME_ROOT "" CACHE PATH "ONNX Runtime root")
  if(NOT ONNXRUNTIME_ROOT)
    message(FATAL_ERROR "Set ONNXRUNTIME_ROOT to build vision_front")
  endif()

  add_executable(vision_front src/vision_front_main.cpp)
  target_include_directories(vision_front PRIVATE
    ${GST_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${ONNXRUNTIME_ROOT}/include)
  target_link_libraries(vision_front
    gst_receiver_lib
    stereo_splitter_lib
    detection_node_lib
    yolo_infer
    ${GST_LIBRARIES}
    ${OpenCV_LIBS}
    ${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so)
  ament_target_dependencies(vision_front
    rclcpp
    sensor_msgs
    robot_interfaces
    gst_receiver
    stereo_splitter
    detection_node)
  install(TARGETS vision_front DESTINATION lib/${PROJECT_NAME})
endif()

set(MOCK_UART_BRIDGE "${CMAKE_CURRENT_SOURCE_DIR}/../../../scripts/mock_uart_bridge.py")
if(EXISTS "${MOCK_UART_BRIDGE}")
  install(PROGRAMS
    "${MOCK_UART_BRIDGE}"
    DESTINATION lib/${PROJECT_NAME}
    RENAME mock_uart_bridge)
endif()

ament_package()
