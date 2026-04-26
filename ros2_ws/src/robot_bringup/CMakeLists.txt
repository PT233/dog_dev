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

# Find library files in installed locations
find_library(GST_RECEIVER_LIB NAMES gst_receiver_lib PATHS /home/peter/dog/dog_dev/install/gst_receiver/lib REQUIRED NO_DEFAULT_PATH)
find_library(STEREO_SPLITTER_LIB NAMES stereo_splitter_lib PATHS /home/peter/dog/dog_dev/install/stereo_splitter/lib REQUIRED NO_DEFAULT_PATH)
find_library(DETECTION_NODE_LIB NAMES detection_node_lib PATHS /home/peter/dog/dog_dev/install/detection_node/lib REQUIRED NO_DEFAULT_PATH)
find_library(YOLO_INFER_LIB NAMES yolo_infer PATHS /home/peter/dog/dog_dev/install/detection_node/lib REQUIRED NO_DEFAULT_PATH)

# Vision front intra-process executable
add_executable(vision_front src/vision_front_main.cpp)
target_include_directories(vision_front PRIVATE
  ${GST_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${ONNXRUNTIME_ROOT}/include)

# Link to libraries
target_link_libraries(vision_front
  ${GST_RECEIVER_LIB}
  ${STEREO_SPLITTER_LIB}
  ${DETECTION_NODE_LIB}
  ${YOLO_INFER_LIB}
  ${GST_LIBRARIES} ${OpenCV_LIBS}
  ${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so
)
ament_target_dependencies(vision_front
  rclcpp sensor_msgs robot_interfaces
  gst_receiver stereo_splitter detection_node
)
install(TARGETS vision_front DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

# Install Python scripts as executables
install(PROGRAMS
  ../../../scripts/mock_uart_bridge.py
  DESTINATION lib/${PROJECT_NAME}
  RENAME mock_uart_bridge
)

ament_package()
