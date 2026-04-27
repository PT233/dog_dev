# generated from ament/cmake/core/templates/nameConfig.cmake.in

# prevent multiple inclusion
if(_detection_node_CONFIG_INCLUDED)
  # ensure to keep the found flag the same
  if(NOT DEFINED detection_node_FOUND)
    # explicitly set it to FALSE, otherwise CMake will set it to TRUE
    set(detection_node_FOUND FALSE)
  elseif(NOT detection_node_FOUND)
    # use separate condition to avoid uninitialized variable warning
    set(detection_node_FOUND FALSE)
  endif()
  return()
endif()
set(_detection_node_CONFIG_INCLUDED TRUE)

# output package information
if(NOT detection_node_FIND_QUIETLY)
  message(STATUS "Found detection_node: 0.0.0 (${detection_node_DIR})")
endif()

# warn when using a deprecated package
if(NOT "" STREQUAL "")
  set(_msg "Package 'detection_node' is deprecated")
  # append custom deprecation text if available
  if(NOT "" STREQUAL "TRUE")
    set(_msg "${_msg} ()")
  endif()
  # optionally quiet the deprecation message
  if(NOT detection_node_DEPRECATED_QUIET)
    message(DEPRECATION "${_msg}")
  endif()
endif()

# flag package as ament-based to distinguish it after being find_package()-ed
set(detection_node_FOUND_AMENT_PACKAGE TRUE)

# include all config extra files
set(_extras "ament_cmake_export_libraries-extras.cmake;ament_cmake_export_include_directories-extras.cmake")
foreach(_extra ${_extras})
  include("${detection_node_DIR}/${_extra}")
endforeach()
