// generated from rosidl_typesupport_cpp/resource/idl__type_support.cpp.em
// with input from robot_interfaces:srv/CalibrateCenter.idl
// generated code does not contain a copyright notice

#include "cstddef"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "robot_interfaces/srv/detail/calibrate_center__functions.h"
#include "robot_interfaces/srv/detail/calibrate_center__struct.hpp"
#include "rosidl_typesupport_cpp/identifier.hpp"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_c/type_support_map.h"
#include "rosidl_typesupport_cpp/message_type_support_dispatch.hpp"
#include "rosidl_typesupport_cpp/visibility_control.h"
#include "rosidl_typesupport_interface/macros.h"

namespace robot_interfaces
{

namespace srv
{

namespace rosidl_typesupport_cpp
{

typedef struct _CalibrateCenter_Request_type_support_ids_t
{
  const char * typesupport_identifier[2];
} _CalibrateCenter_Request_type_support_ids_t;

static const _CalibrateCenter_Request_type_support_ids_t _CalibrateCenter_Request_message_typesupport_ids = {
  {
    "rosidl_typesupport_fastrtps_cpp",  // ::rosidl_typesupport_fastrtps_cpp::typesupport_identifier,
    "rosidl_typesupport_introspection_cpp",  // ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  }
};

typedef struct _CalibrateCenter_Request_type_support_symbol_names_t
{
  const char * symbol_name[2];
} _CalibrateCenter_Request_type_support_symbol_names_t;

#define STRINGIFY_(s) #s
#define STRINGIFY(s) STRINGIFY_(s)

static const _CalibrateCenter_Request_type_support_symbol_names_t _CalibrateCenter_Request_message_typesupport_symbol_names = {
  {
    STRINGIFY(ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_cpp, robot_interfaces, srv, CalibrateCenter_Request)),
    STRINGIFY(ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, robot_interfaces, srv, CalibrateCenter_Request)),
  }
};

typedef struct _CalibrateCenter_Request_type_support_data_t
{
  void * data[2];
} _CalibrateCenter_Request_type_support_data_t;

static _CalibrateCenter_Request_type_support_data_t _CalibrateCenter_Request_message_typesupport_data = {
  {
    0,  // will store the shared library later
    0,  // will store the shared library later
  }
};

static const type_support_map_t _CalibrateCenter_Request_message_typesupport_map = {
  2,
  "robot_interfaces",
  &_CalibrateCenter_Request_message_typesupport_ids.typesupport_identifier[0],
  &_CalibrateCenter_Request_message_typesupport_symbol_names.symbol_name[0],
  &_CalibrateCenter_Request_message_typesupport_data.data[0],
};

static const rosidl_message_type_support_t CalibrateCenter_Request_message_type_support_handle = {
  ::rosidl_typesupport_cpp::typesupport_identifier,
  reinterpret_cast<const type_support_map_t *>(&_CalibrateCenter_Request_message_typesupport_map),
  ::rosidl_typesupport_cpp::get_message_typesupport_handle_function,
  &robot_interfaces__srv__CalibrateCenter_Request__get_type_hash,
  &robot_interfaces__srv__CalibrateCenter_Request__get_type_description,
  &robot_interfaces__srv__CalibrateCenter_Request__get_type_description_sources,
};

}  // namespace rosidl_typesupport_cpp

}  // namespace srv

}  // namespace robot_interfaces

namespace rosidl_typesupport_cpp
{

template<>
ROSIDL_TYPESUPPORT_CPP_PUBLIC
const rosidl_message_type_support_t *
get_message_type_support_handle<robot_interfaces::srv::CalibrateCenter_Request>()
{
  return &::robot_interfaces::srv::rosidl_typesupport_cpp::CalibrateCenter_Request_message_type_support_handle;
}

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_cpp, robot_interfaces, srv, CalibrateCenter_Request)() {
  return get_message_type_support_handle<robot_interfaces::srv::CalibrateCenter_Request>();
}

#ifdef __cplusplus
}
#endif
}  // namespace rosidl_typesupport_cpp

// already included above
// #include "cstddef"
// already included above
// #include "rosidl_runtime_c/message_type_support_struct.h"
// already included above
// #include "robot_interfaces/srv/detail/calibrate_center__functions.h"
// already included above
// #include "robot_interfaces/srv/detail/calibrate_center__struct.hpp"
// already included above
// #include "rosidl_typesupport_cpp/identifier.hpp"
// already included above
// #include "rosidl_typesupport_cpp/message_type_support.hpp"
// already included above
// #include "rosidl_typesupport_c/type_support_map.h"
// already included above
// #include "rosidl_typesupport_cpp/message_type_support_dispatch.hpp"
// already included above
// #include "rosidl_typesupport_cpp/visibility_control.h"
// already included above
// #include "rosidl_typesupport_interface/macros.h"

namespace robot_interfaces
{

namespace srv
{

namespace rosidl_typesupport_cpp
{

typedef struct _CalibrateCenter_Response_type_support_ids_t
{
  const char * typesupport_identifier[2];
} _CalibrateCenter_Response_type_support_ids_t;

static const _CalibrateCenter_Response_type_support_ids_t _CalibrateCenter_Response_message_typesupport_ids = {
  {
    "rosidl_typesupport_fastrtps_cpp",  // ::rosidl_typesupport_fastrtps_cpp::typesupport_identifier,
    "rosidl_typesupport_introspection_cpp",  // ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  }
};

typedef struct _CalibrateCenter_Response_type_support_symbol_names_t
{
  const char * symbol_name[2];
} _CalibrateCenter_Response_type_support_symbol_names_t;

#define STRINGIFY_(s) #s
#define STRINGIFY(s) STRINGIFY_(s)

static const _CalibrateCenter_Response_type_support_symbol_names_t _CalibrateCenter_Response_message_typesupport_symbol_names = {
  {
    STRINGIFY(ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_cpp, robot_interfaces, srv, CalibrateCenter_Response)),
    STRINGIFY(ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, robot_interfaces, srv, CalibrateCenter_Response)),
  }
};

typedef struct _CalibrateCenter_Response_type_support_data_t
{
  void * data[2];
} _CalibrateCenter_Response_type_support_data_t;

static _CalibrateCenter_Response_type_support_data_t _CalibrateCenter_Response_message_typesupport_data = {
  {
    0,  // will store the shared library later
    0,  // will store the shared library later
  }
};

static const type_support_map_t _CalibrateCenter_Response_message_typesupport_map = {
  2,
  "robot_interfaces",
  &_CalibrateCenter_Response_message_typesupport_ids.typesupport_identifier[0],
  &_CalibrateCenter_Response_message_typesupport_symbol_names.symbol_name[0],
  &_CalibrateCenter_Response_message_typesupport_data.data[0],
};

static const rosidl_message_type_support_t CalibrateCenter_Response_message_type_support_handle = {
  ::rosidl_typesupport_cpp::typesupport_identifier,
  reinterpret_cast<const type_support_map_t *>(&_CalibrateCenter_Response_message_typesupport_map),
  ::rosidl_typesupport_cpp::get_message_typesupport_handle_function,
  &robot_interfaces__srv__CalibrateCenter_Response__get_type_hash,
  &robot_interfaces__srv__CalibrateCenter_Response__get_type_description,
  &robot_interfaces__srv__CalibrateCenter_Response__get_type_description_sources,
};

}  // namespace rosidl_typesupport_cpp

}  // namespace srv

}  // namespace robot_interfaces

namespace rosidl_typesupport_cpp
{

template<>
ROSIDL_TYPESUPPORT_CPP_PUBLIC
const rosidl_message_type_support_t *
get_message_type_support_handle<robot_interfaces::srv::CalibrateCenter_Response>()
{
  return &::robot_interfaces::srv::rosidl_typesupport_cpp::CalibrateCenter_Response_message_type_support_handle;
}

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_cpp, robot_interfaces, srv, CalibrateCenter_Response)() {
  return get_message_type_support_handle<robot_interfaces::srv::CalibrateCenter_Response>();
}

#ifdef __cplusplus
}
#endif
}  // namespace rosidl_typesupport_cpp

// already included above
// #include "cstddef"
// already included above
// #include "rosidl_runtime_c/message_type_support_struct.h"
// already included above
// #include "robot_interfaces/srv/detail/calibrate_center__functions.h"
// already included above
// #include "robot_interfaces/srv/detail/calibrate_center__struct.hpp"
// already included above
// #include "rosidl_typesupport_cpp/identifier.hpp"
// already included above
// #include "rosidl_typesupport_cpp/message_type_support.hpp"
// already included above
// #include "rosidl_typesupport_c/type_support_map.h"
// already included above
// #include "rosidl_typesupport_cpp/message_type_support_dispatch.hpp"
// already included above
// #include "rosidl_typesupport_cpp/visibility_control.h"
// already included above
// #include "rosidl_typesupport_interface/macros.h"

namespace robot_interfaces
{

namespace srv
{

namespace rosidl_typesupport_cpp
{

typedef struct _CalibrateCenter_Event_type_support_ids_t
{
  const char * typesupport_identifier[2];
} _CalibrateCenter_Event_type_support_ids_t;

static const _CalibrateCenter_Event_type_support_ids_t _CalibrateCenter_Event_message_typesupport_ids = {
  {
    "rosidl_typesupport_fastrtps_cpp",  // ::rosidl_typesupport_fastrtps_cpp::typesupport_identifier,
    "rosidl_typesupport_introspection_cpp",  // ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  }
};

typedef struct _CalibrateCenter_Event_type_support_symbol_names_t
{
  const char * symbol_name[2];
} _CalibrateCenter_Event_type_support_symbol_names_t;

#define STRINGIFY_(s) #s
#define STRINGIFY(s) STRINGIFY_(s)

static const _CalibrateCenter_Event_type_support_symbol_names_t _CalibrateCenter_Event_message_typesupport_symbol_names = {
  {
    STRINGIFY(ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_cpp, robot_interfaces, srv, CalibrateCenter_Event)),
    STRINGIFY(ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, robot_interfaces, srv, CalibrateCenter_Event)),
  }
};

typedef struct _CalibrateCenter_Event_type_support_data_t
{
  void * data[2];
} _CalibrateCenter_Event_type_support_data_t;

static _CalibrateCenter_Event_type_support_data_t _CalibrateCenter_Event_message_typesupport_data = {
  {
    0,  // will store the shared library later
    0,  // will store the shared library later
  }
};

static const type_support_map_t _CalibrateCenter_Event_message_typesupport_map = {
  2,
  "robot_interfaces",
  &_CalibrateCenter_Event_message_typesupport_ids.typesupport_identifier[0],
  &_CalibrateCenter_Event_message_typesupport_symbol_names.symbol_name[0],
  &_CalibrateCenter_Event_message_typesupport_data.data[0],
};

static const rosidl_message_type_support_t CalibrateCenter_Event_message_type_support_handle = {
  ::rosidl_typesupport_cpp::typesupport_identifier,
  reinterpret_cast<const type_support_map_t *>(&_CalibrateCenter_Event_message_typesupport_map),
  ::rosidl_typesupport_cpp::get_message_typesupport_handle_function,
  &robot_interfaces__srv__CalibrateCenter_Event__get_type_hash,
  &robot_interfaces__srv__CalibrateCenter_Event__get_type_description,
  &robot_interfaces__srv__CalibrateCenter_Event__get_type_description_sources,
};

}  // namespace rosidl_typesupport_cpp

}  // namespace srv

}  // namespace robot_interfaces

namespace rosidl_typesupport_cpp
{

template<>
ROSIDL_TYPESUPPORT_CPP_PUBLIC
const rosidl_message_type_support_t *
get_message_type_support_handle<robot_interfaces::srv::CalibrateCenter_Event>()
{
  return &::robot_interfaces::srv::rosidl_typesupport_cpp::CalibrateCenter_Event_message_type_support_handle;
}

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_cpp, robot_interfaces, srv, CalibrateCenter_Event)() {
  return get_message_type_support_handle<robot_interfaces::srv::CalibrateCenter_Event>();
}

#ifdef __cplusplus
}
#endif
}  // namespace rosidl_typesupport_cpp

// already included above
// #include "cstddef"
#include "rosidl_runtime_c/service_type_support_struct.h"
#include "rosidl_typesupport_cpp/service_type_support.hpp"
// already included above
// #include "robot_interfaces/srv/detail/calibrate_center__struct.hpp"
// already included above
// #include "rosidl_typesupport_cpp/identifier.hpp"
// already included above
// #include "rosidl_typesupport_c/type_support_map.h"
#include "rosidl_typesupport_cpp/service_type_support_dispatch.hpp"
// already included above
// #include "rosidl_typesupport_cpp/visibility_control.h"
// already included above
// #include "rosidl_typesupport_interface/macros.h"

namespace robot_interfaces
{

namespace srv
{

namespace rosidl_typesupport_cpp
{

typedef struct _CalibrateCenter_type_support_ids_t
{
  const char * typesupport_identifier[2];
} _CalibrateCenter_type_support_ids_t;

static const _CalibrateCenter_type_support_ids_t _CalibrateCenter_service_typesupport_ids = {
  {
    "rosidl_typesupport_fastrtps_cpp",  // ::rosidl_typesupport_fastrtps_cpp::typesupport_identifier,
    "rosidl_typesupport_introspection_cpp",  // ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  }
};

typedef struct _CalibrateCenter_type_support_symbol_names_t
{
  const char * symbol_name[2];
} _CalibrateCenter_type_support_symbol_names_t;
#define STRINGIFY_(s) #s
#define STRINGIFY(s) STRINGIFY_(s)

static const _CalibrateCenter_type_support_symbol_names_t _CalibrateCenter_service_typesupport_symbol_names = {
  {
    STRINGIFY(ROSIDL_TYPESUPPORT_INTERFACE__SERVICE_SYMBOL_NAME(rosidl_typesupport_fastrtps_cpp, robot_interfaces, srv, CalibrateCenter)),
    STRINGIFY(ROSIDL_TYPESUPPORT_INTERFACE__SERVICE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, robot_interfaces, srv, CalibrateCenter)),
  }
};

typedef struct _CalibrateCenter_type_support_data_t
{
  void * data[2];
} _CalibrateCenter_type_support_data_t;

static _CalibrateCenter_type_support_data_t _CalibrateCenter_service_typesupport_data = {
  {
    0,  // will store the shared library later
    0,  // will store the shared library later
  }
};

static const type_support_map_t _CalibrateCenter_service_typesupport_map = {
  2,
  "robot_interfaces",
  &_CalibrateCenter_service_typesupport_ids.typesupport_identifier[0],
  &_CalibrateCenter_service_typesupport_symbol_names.symbol_name[0],
  &_CalibrateCenter_service_typesupport_data.data[0],
};

static const rosidl_service_type_support_t CalibrateCenter_service_type_support_handle = {
  ::rosidl_typesupport_cpp::typesupport_identifier,
  reinterpret_cast<const type_support_map_t *>(&_CalibrateCenter_service_typesupport_map),
  ::rosidl_typesupport_cpp::get_service_typesupport_handle_function,
  ::rosidl_typesupport_cpp::get_message_type_support_handle<robot_interfaces::srv::CalibrateCenter_Request>(),
  ::rosidl_typesupport_cpp::get_message_type_support_handle<robot_interfaces::srv::CalibrateCenter_Response>(),
  ::rosidl_typesupport_cpp::get_message_type_support_handle<robot_interfaces::srv::CalibrateCenter_Event>(),
  &::rosidl_typesupport_cpp::service_create_event_message<robot_interfaces::srv::CalibrateCenter>,
  &::rosidl_typesupport_cpp::service_destroy_event_message<robot_interfaces::srv::CalibrateCenter>,
  &robot_interfaces__srv__CalibrateCenter__get_type_hash,
  &robot_interfaces__srv__CalibrateCenter__get_type_description,
  &robot_interfaces__srv__CalibrateCenter__get_type_description_sources,
};

}  // namespace rosidl_typesupport_cpp

}  // namespace srv

}  // namespace robot_interfaces

namespace rosidl_typesupport_cpp
{

template<>
ROSIDL_TYPESUPPORT_CPP_PUBLIC
const rosidl_service_type_support_t *
get_service_type_support_handle<robot_interfaces::srv::CalibrateCenter>()
{
  return &::robot_interfaces::srv::rosidl_typesupport_cpp::CalibrateCenter_service_type_support_handle;
}

}  // namespace rosidl_typesupport_cpp

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_CPP_PUBLIC
const rosidl_service_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__SERVICE_SYMBOL_NAME(rosidl_typesupport_cpp, robot_interfaces, srv, CalibrateCenter)() {
  return ::rosidl_typesupport_cpp::get_service_type_support_handle<robot_interfaces::srv::CalibrateCenter>();
}

#ifdef __cplusplus
}
#endif
