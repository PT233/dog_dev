cmake_minimum_required(VERSION 3.8)
project(robot_bringup)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(robot_interfaces REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(gst_receiver REQUIRED)
find_package(stereo_splitter REQUIRED)
find_package(detection_node REQUIRED)

pkg_check_modules(GST REQUIRED gstreamer-1.0 gstreamer-app-1.0)

# ONNX Runtime configuration
set(ONNXRUNTIME_ROOT "/home/peter/onnxruntime-gpu" CACHE PATH "ONNX Runtime root")
set(INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib")

# Vision front intra-process executable
add_executable(vision_front src/vision_front_main.cpp)
target_include_directories(vision_front PRIVATE
  ${GST_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS}
  ${gst_receiver_INCLUDE_DIRS}
  ${stereo_splitter_INCLUDE_DIRS}
  ${detection_node_INCLUDE_DIRS}
  ${ONNXRUNTIME_ROOT}/include
)
target_link_libraries(vision_front
  -Wl,--whole-archive detection_node_lib -Wl,--no-whole-archive
  gst_receiver_lib stereo_splitter_lib yolo_infer
  ${GST_LIBRARIES} ${OpenCV_LIBS}
  ${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so
)
ament_target_dependencies(vision_front rclcpp sensor_msgs robot_interfaces)
install(TARGETS vision_front DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

# Install Python scripts as executables
install(PROGRAMS
  ../../../scripts/mock_uart_bridge.py
  DESTINATION lib/${PROJECT_NAME}
  RENAME mock_uart_bridge
)

ament_package()
