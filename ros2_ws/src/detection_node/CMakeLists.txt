cmake_minimum_required(VERSION 3.8)
project(detection_node)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 依赖：OpenCV 做图像预处理/NMS，ONNX Runtime 由 ONNXRUNTIME_ROOT 显式指定。
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

# yolo_infer 封装模型加载、推理、预处理和后处理，供节点和测试复用。
add_library(yolo_infer src/yolo_infer.cpp)
target_include_directories(yolo_infer PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${ONNXRUNTIME_ROOT}/include)
target_include_directories(yolo_infer PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
target_link_libraries(yolo_infer ${OpenCV_LIBS} ${ONNXRUNTIME_LIB})

# YOLO 推理测试程序，用于单独验证模型文件和 ONNX Runtime 环境。
add_executable(test_yolo_infer src/test_yolo_infer.cpp)
target_link_libraries(test_yolo_infer yolo_infer)

# 检测节点主体编成库，robot_bringup 的 vision_front 可直接同进程链接。
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

# 单独运行的检测节点可执行文件。
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

# 检测可视化节点，用于把 tracked_objects 叠加到原始图像。
add_executable(detection_viz_node_exe src/detection_viz_node.cpp src/detection_viz_main.cpp)
ament_target_dependencies(detection_viz_node_exe
  rclcpp
  sensor_msgs
  OpenCV
  robot_interfaces
)
target_include_directories(detection_viz_node_exe PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_include_directories(detection_viz_node_exe PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
target_link_libraries(detection_viz_node_exe ${OpenCV_LIBS})

set(_onnxruntime_targets
  yolo_infer
  detection_node_lib
  detection_node_exe
  test_yolo_infer)
foreach(_target IN LISTS _onnxruntime_targets)
  # 写入 RPATH，使运行时能找到 ONNX Runtime 动态库。
  set_target_properties(${_target} PROPERTIES
    BUILD_RPATH "${ONNXRUNTIME_LIB_DIR}"
    INSTALL_RPATH "${ONNXRUNTIME_LIB_DIR}")
endforeach()

# 安装头文件，供 robot_bringup 同进程可执行文件链接。
install(DIRECTORY include/ DESTINATION include)

# 安装节点可执行文件。
install(TARGETS
  detection_node_exe
  detection_viz_node_exe
  DESTINATION lib/${PROJECT_NAME})

# 安装可复用库目标。
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

# 安装测试可执行文件，便于部署后验证推理环境。
install(TARGETS test_yolo_infer
  DESTINATION lib/${PROJECT_NAME})

# 安装参数文件。
install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME})

# 安装启动文件。
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})
