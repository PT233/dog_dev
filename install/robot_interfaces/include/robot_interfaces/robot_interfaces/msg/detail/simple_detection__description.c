// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from robot_interfaces:msg/SimpleDetection.idl
// generated code does not contain a copyright notice

#include "robot_interfaces/msg/detail/simple_detection__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
const rosidl_type_hash_t *
robot_interfaces__msg__SimpleDetection__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0x03, 0xae, 0x58, 0x8c, 0xe2, 0x02, 0x54, 0x6c,
      0x3a, 0xff, 0x44, 0x6d, 0x9d, 0x53, 0x94, 0x4c,
      0x65, 0xf9, 0x72, 0x36, 0xa9, 0x6b, 0xc4, 0x89,
      0x21, 0x17, 0x22, 0xa1, 0xad, 0x5a, 0xee, 0x7f,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types

// Hashes for external referenced types
#ifndef NDEBUG
#endif

static char robot_interfaces__msg__SimpleDetection__TYPE_NAME[] = "robot_interfaces/msg/SimpleDetection";

// Define type names, field names, and default values
static char robot_interfaces__msg__SimpleDetection__FIELD_NAME__center_x[] = "center_x";
static char robot_interfaces__msg__SimpleDetection__FIELD_NAME__center_y[] = "center_y";
static char robot_interfaces__msg__SimpleDetection__FIELD_NAME__width[] = "width";
static char robot_interfaces__msg__SimpleDetection__FIELD_NAME__height[] = "height";
static char robot_interfaces__msg__SimpleDetection__FIELD_NAME__confidence[] = "confidence";
static char robot_interfaces__msg__SimpleDetection__FIELD_NAME__class_id[] = "class_id";
static char robot_interfaces__msg__SimpleDetection__FIELD_NAME__track_id[] = "track_id";

static rosidl_runtime_c__type_description__Field robot_interfaces__msg__SimpleDetection__FIELDS[] = {
  {
    {robot_interfaces__msg__SimpleDetection__FIELD_NAME__center_x, 8, 8},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__SimpleDetection__FIELD_NAME__center_y, 8, 8},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__SimpleDetection__FIELD_NAME__width, 5, 5},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__SimpleDetection__FIELD_NAME__height, 6, 6},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__SimpleDetection__FIELD_NAME__confidence, 10, 10},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__SimpleDetection__FIELD_NAME__class_id, 8, 8},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_INT32,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__SimpleDetection__FIELD_NAME__track_id, 8, 8},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_STRING,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
};

const rosidl_runtime_c__type_description__TypeDescription *
robot_interfaces__msg__SimpleDetection__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {robot_interfaces__msg__SimpleDetection__TYPE_NAME, 36, 36},
      {robot_interfaces__msg__SimpleDetection__FIELDS, 7, 7},
    },
    {NULL, 0, 0},
  };
  if (!constructed) {
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "float32 center_x\n"
  "float32 center_y\n"
  "float32 width\n"
  "float32 height\n"
  "float32 confidence\n"
  "int32 class_id\n"
  "string track_id";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
robot_interfaces__msg__SimpleDetection__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {robot_interfaces__msg__SimpleDetection__TYPE_NAME, 36, 36},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 113, 113},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
robot_interfaces__msg__SimpleDetection__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[1];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 1, 1};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *robot_interfaces__msg__SimpleDetection__get_individual_type_description_source(NULL),
    constructed = true;
  }
  return &source_sequence;
}
