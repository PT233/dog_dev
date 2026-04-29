cmake_minimum_required(VERSION 3.8)
project(robot_bringup)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

# 安装组合启动文件，bringup 包本身主要负责把各功能包拼起来。
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

# vision_front 是可选的同进程视觉前端，默认关闭以降低普通构建依赖。
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

  # ONNX Runtime 没有通过 ament 包提供，构建该可执行文件时需要显式指定根目录。
  set(ONNXRUNTIME_ROOT "" CACHE PATH "ONNX Runtime root")
  if(NOT ONNXRUNTIME_ROOT)
    message(FATAL_ERROR "Set ONNXRUNTIME_ROOT to build vision_front")
  endif()

  # 将接收、裁剪、检测三个节点链接进同一进程，减少图像消息拷贝。
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
  # 有 mock 脚本时安装为可执行目标，供无 STM32 硬件的集成测试 launch 使用。
  install(PROGRAMS
    "${MOCK_UART_BRIDGE}"
    DESTINATION lib/${PROJECT_NAME}
    RENAME mock_uart_bridge)
endif()

if(BUILD_TESTING)
  enable_testing()
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
