cmake_minimum_required(VERSION 3.8)
project(imu_node)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(Threads REQUIRED)

# 读取内核驱动 mpu6050_lab 的字符设备，发布 sensor_msgs/Imu。
add_executable(imu_node src/imu_node.cpp)
target_compile_features(imu_node PRIVATE cxx_std_17)
# 仓库根目录：shared/ 下的公共头，以及 kernel/mpu6050 下的驱动 uapi 头。
target_include_directories(imu_node PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
target_link_libraries(imu_node Threads::Threads)
ament_target_dependencies(imu_node rclcpp sensor_msgs)

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})
install(TARGETS imu_node DESTINATION lib/${PROJECT_NAME})

ament_package()
