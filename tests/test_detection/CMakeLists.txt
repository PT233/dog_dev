cmake_minimum_required(VERSION 3.10)
project(test_onnx_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(OpenCV REQUIRED)

# ONNX Runtime - from downloaded SDK
set(ONNXRUNTIME_ROOT "/tmp/onnxruntime-linux-x64-gpu-1.25.0")

set(ONNXRUNTIME_INCLUDE_DIRS "${ONNXRUNTIME_ROOT}/include")
set(ONNXRUNTIME_LIB_DIR "${ONNXRUNTIME_ROOT}/lib")

include_directories(${OPENCV_INCLUDE_DIRS})
include_directories(${ONNXRUNTIME_INCLUDE_DIRS})

# Test executable
add_executable(test_onnx_cpp test_onnx_cpp.cpp)

# Link libraries
target_link_libraries(test_onnx_cpp
    ${OPENCV_LIBS}
    ${ONNXRUNTIME_LIB_DIR}/libonnxruntime.so
)

# Set library search path
set_target_properties(test_onnx_cpp PROPERTIES
    LINK_FLAGS "-Wl,-rpath,${ONNXRUNTIME_LIB_DIR}"
)

# Include directories
target_include_directories(test_onnx_cpp PRIVATE
    ${OPENCV_INCLUDE_DIRS}
    ${ONNXRUNTIME_INCLUDE_DIRS}
)
