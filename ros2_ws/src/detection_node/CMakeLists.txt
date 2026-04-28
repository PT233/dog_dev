cmake_minimum_required(VERSION 3.8)
project(detection_node)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(OpenCV REQUIRED)
find_package(robot_interfaces REQUIRED)

set(ONNXRUNTIME_ROOT "$ENV{ONNXRUNTIME_ROOT}" CACHE PATH "ONNX Runtime root")
if(NOT ONNXRUNTIME_ROOT)
  message(FATAL_ERROR "Set ONNXRUNTIME_ROOT to the ONNX Runtime SDK root")
endif()
if(NOT EXISTS "${ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h")
  message(FATAL_ERROR "ONNXRUNTIME_ROOT does not contain include/onnxruntime_cxx_api.h: ${ONNXRUNTIME_ROOT}")
endif()
if(NOT EXISTS "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so")
  message(FATAL_ERROR "ONNXRUNTIME_ROOT does not contain lib/libonnxruntime.so: ${ONNXRUNTIME_ROOT}")
endif()
set(ONNXRUNTIME_LIB_DIR "${ONNXRUNTIME_ROOT}/lib")
set(ONNXRUNTIME_LIB "${ONNXRUNTIME_LIB_DIR}/libonnxruntime.so")

# Create yolo_infer library
add_library(yolo_infer src/yolo_infer.cpp)
target_include_directories(yolo_infer PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${ONNXRUNTIME_ROOT}/include)
target_include_directories(yolo_infer PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
target_link_libraries(yolo_infer ${OpenCV_LIBS} ${ONNXRUNTIME_LIB})

# Test executable for yolo_infer
add_executable(test_yolo_infer src/test_yolo_infer.cpp)
target_link_libraries(test_yolo_infer yolo_infer)

# Detection node library (for intra-process use)
add_library(detection_node_lib src/detection_node.cpp)
target_include_directories(detection_node_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
ament_target_dependencies(detection_node_lib
  rclcpp
  sensor_msgs
  OpenCV
  robot_interfaces
)
target_link_libraries(detection_node_lib yolo_infer)

# Detection node executable
add_executable(detection_node_exe src/detection_node_main.cpp)
ament_target_dependencies(detection_node_exe
  rclcpp
  sensor_msgs
  OpenCV
  robot_interfaces
)
target_include_directories(detection_node_exe PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_include_directories(detection_node_exe PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
target_link_libraries(detection_node_exe detection_node_lib)

# Detection visualization node executable
add_executable(detection_viz_node_exe src/detection_viz_node.cpp src/detection_viz_main.cpp)
ament_target_dependencies(detection_viz_node_exe
  rclcpp
  sensor_msgs
  robot_interfaces
)
target_include_directories(detection_viz_node_exe PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_include_directories(detection_viz_node_exe PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

set(_onnxruntime_targets
  yolo_infer
  detection_node_lib
  detection_node_exe
  test_yolo_infer)
foreach(_target IN LISTS _onnxruntime_targets)
  set_target_properties(${_target} PROPERTIES
    BUILD_RPATH "${ONNXRUNTIME_LIB_DIR}"
    INSTALL_RPATH "${ONNXRUNTIME_LIB_DIR}")
endforeach()

# Install headers
install(DIRECTORY include/ DESTINATION include)

# Install executables
install(TARGETS
  detection_node_exe
  detection_viz_node_exe
  DESTINATION lib/${PROJECT_NAME})

# Install libraries
install(TARGETS detection_node_lib yolo_infer
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

ament_export_libraries(detection_node_lib)
ament_export_include_directories(include)
ament_package()

# Install executables
install(TARGETS test_yolo_infer
  DESTINATION lib/${PROJECT_NAME})

# Install config files
install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME})

# Install launch files
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})
