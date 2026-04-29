// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from robot_interfaces:msg/TargetInfo.idl
// generated code does not contain a copyright notice

#include "robot_interfaces/msg/detail/target_info__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_robot_interfaces
const rosidl_type_hash_t *
robot_interfaces__msg__TargetInfo__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0x61, 0x49, 0x02, 0x50, 0xde, 0x0f, 0x15, 0xef,
      0xde, 0x01, 0x7d, 0xc2, 0xf9, 0xdf, 0xeb, 0xb2,
      0x06, 0xf6, 0xfb, 0x23, 0xeb, 0x89, 0xba, 0x59,
      0x78, 0xdf, 0x21, 0x2a, 0x45, 0xde, 0xab, 0xce,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types
#include "std_msgs/msg/detail/header__functions.h"
#include "builtin_interfaces/msg/detail/time__functions.h"

// Hashes for external referenced types
#ifndef NDEBUG
static const rosidl_type_hash_t builtin_interfaces__msg__Time__EXPECTED_HASH = {1, {
    0xb1, 0x06, 0x23, 0x5e, 0x25, 0xa4, 0xc5, 0xed,
    0x35, 0x09, 0x8a, 0xa0, 0xa6, 0x1a, 0x3e, 0xe9,
    0xc9, 0xb1, 0x8d, 0x19, 0x7f, 0x39, 0x8b, 0x0e,
    0x42, 0x06, 0xce, 0xa9, 0xac, 0xf9, 0xc1, 0x97,
  }};
static const rosidl_type_hash_t std_msgs__msg__Header__EXPECTED_HASH = {1, {
    0xf4, 0x9f, 0xb3, 0xae, 0x2c, 0xf0, 0x70, 0xf7,
    0x93, 0x64, 0x5f, 0xf7, 0x49, 0x68, 0x3a, 0xc6,
    0xb0, 0x62, 0x03, 0xe4, 0x1c, 0x89, 0x1e, 0x17,
    0x70, 0x1b, 0x1c, 0xb5, 0x97, 0xce, 0x6a, 0x01,
  }};
#endif

static char robot_interfaces__msg__TargetInfo__TYPE_NAME[] = "robot_interfaces/msg/TargetInfo";
static char builtin_interfaces__msg__Time__TYPE_NAME[] = "builtin_interfaces/msg/Time";
static char std_msgs__msg__Header__TYPE_NAME[] = "std_msgs/msg/Header";

// Define type names, field names, and default values
static char robot_interfaces__msg__TargetInfo__FIELD_NAME__header[] = "header";
static char robot_interfaces__msg__TargetInfo__FIELD_NAME__class_name[] = "class_name";
static char robot_interfaces__msg__TargetInfo__FIELD_NAME__track_id[] = "track_id";
static char robot_interfaces__msg__TargetInfo__FIELD_NAME__confidence[] = "confidence";
static char robot_interfaces__msg__TargetInfo__FIELD_NAME__bounding_box_center_x[] = "bounding_box_center_x";
static char robot_interfaces__msg__TargetInfo__FIELD_NAME__bounding_box_center_y[] = "bounding_box_center_y";
static char robot_interfaces__msg__TargetInfo__FIELD_NAME__bounding_box_width[] = "bounding_box_width";
static char robot_interfaces__msg__TargetInfo__FIELD_NAME__bounding_box_height[] = "bounding_box_height";
static char robot_interfaces__msg__TargetInfo__FIELD_NAME__is_locked[] = "is_locked";

static rosidl_runtime_c__type_description__Field robot_interfaces__msg__TargetInfo__FIELDS[] = {
  {
    {robot_interfaces__msg__TargetInfo__FIELD_NAME__header, 6, 6},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_NESTED_TYPE,
      0,
      0,
      {std_msgs__msg__Header__TYPE_NAME, 19, 19},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__TargetInfo__FIELD_NAME__class_name, 10, 10},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_STRING,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__TargetInfo__FIELD_NAME__track_id, 8, 8},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_STRING,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__TargetInfo__FIELD_NAME__confidence, 10, 10},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__TargetInfo__FIELD_NAME__bounding_box_center_x, 21, 21},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_INT32,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__TargetInfo__FIELD_NAME__bounding_box_center_y, 21, 21},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_INT32,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__TargetInfo__FIELD_NAME__bounding_box_width, 18, 18},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_INT32,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__TargetInfo__FIELD_NAME__bounding_box_height, 19, 19},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_INT32,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {robot_interfaces__msg__TargetInfo__FIELD_NAME__is_locked, 9, 9},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_BOOLEAN,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
};

static rosidl_runtime_c__type_description__IndividualTypeDescription robot_interfaces__msg__TargetInfo__REFERENCED_TYPE_DESCRIPTIONS[] = {
  {
    {builtin_interfaces__msg__Time__TYPE_NAME, 27, 27},
    {NULL, 0, 0},
  },
  {
    {std_msgs__msg__Header__TYPE_NAME, 19, 19},
    {NULL, 0, 0},
  },
};

const rosidl_runtime_c__type_description__TypeDescription *
robot_interfaces__msg__TargetInfo__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {robot_interfaces__msg__TargetInfo__TYPE_NAME, 31, 31},
      {robot_interfaces__msg__TargetInfo__FIELDS, 9, 9},
    },
    {robot_interfaces__msg__TargetInfo__REFERENCED_TYPE_DESCRIPTIONS, 2, 2},
  };
  if (!constructed) {
    assert(0 == memcmp(&builtin_interfaces__msg__Time__EXPECTED_HASH, builtin_interfaces__msg__Time__get_type_hash(NULL), sizeof(rosidl_type_hash_t)));
    description.referenced_type_descriptions.data[0].fields = builtin_interfaces__msg__Time__get_type_description(NULL)->type_description.fields;
    assert(0 == memcmp(&std_msgs__msg__Header__EXPECTED_HASH, std_msgs__msg__Header__get_type_hash(NULL), sizeof(rosidl_type_hash_t)));
    description.referenced_type_descriptions.data[1].fields = std_msgs__msg__Header__get_type_description(NULL)->type_description.fields;
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "# \\xe5\\xbd\\x93\\xe5\\x89\\x8d\\xe7\\x9b\\xae\\xe6\\xa0\\x87\\xe9\\x80\\x89\\xe6\\x8b\\xa9\\xe7\\xbb\\x93\\xe6\\x9e\\x9c\\xef\\xbc\\x8c\\xe9\\xa2\\x84\\xe7\\x95\\x99\\xe7\\xbb\\x99\\xe6\\x9b\\xb4\\xe5\\xae\\x8c\\xe6\\x95\\xb4\\xe7\\x9a\\x84\\xe8\\xa1\\x8c\\xe4\\xb8\\xba/\\xe7\\x8a\\xb6\\xe6\\x80\\x81\\xe6\\x98\\xbe\\xe7\\xa4\\xba\\xe3\\x80\\x82\n"
  "std_msgs/Header header\n"
  "\n"
  "# \\xe7\\x9b\\xae\\xe6\\xa0\\x87\\xe7\\xb1\\xbb\\xe5\\x88\\xab\\xe5\\x90\\x8d\\xe5\\x92\\x8c\\xe8\\xb7\\x9f\\xe8\\xb8\\xaa ID\\xe3\\x80\\x82\n"
  "string class_name\n"
  "string track_id\n"
  "\n"
  "# \\xe5\\xbd\\x93\\xe5\\x89\\x8d\\xe7\\x9b\\xae\\xe6\\xa0\\x87\\xe7\\xbd\\xae\\xe4\\xbf\\xa1\\xe5\\xba\\xa6\\xe5\\x92\\x8c\\xe6\\xa3\\x80\\xe6\\xb5\\x8b\\xe6\\xa1\\x86\\xe5\\x83\\x8f\\xe7\\xb4\\xa0\\xe4\\xbd\\x8d\\xe7\\xbd\\xae\\xe3\\x80\\x82\n"
  "float32 confidence\n"
  "int32 bounding_box_center_x\n"
  "int32 bounding_box_center_y\n"
  "int32 bounding_box_width\n"
  "int32 bounding_box_height\n"
  "\n"
  "# true \\xe8\\xa1\\xa8\\xe7\\xa4\\xba\\xe5\\xbd\\x93\\xe5\\x89\\x8d\\xe8\\xa1\\x8c\\xe4\\xb8\\xba\\xe8\\x8a\\x82\\xe7\\x82\\xb9\\xe5\\xb7\\xb2\\xe7\\xbb\\x8f\\xe9\\x94\\x81\\xe5\\xae\\x9a\\xe4\\xb8\\x80\\xe4\\xb8\\xaa\\xe6\\x9c\\x89\\xe6\\x95\\x88\\xe7\\x9b\\xae\\xe6\\xa0\\x87\\xe3\\x80\\x82\n"
  "bool is_locked";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
robot_interfaces__msg__TargetInfo__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {robot_interfaces__msg__TargetInfo__TYPE_NAME, 31, 31},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 289, 289},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
robot_interfaces__msg__TargetInfo__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[3];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 3, 3};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *robot_interfaces__msg__TargetInfo__get_individual_type_description_source(NULL),
    sources[1] = *builtin_interfaces__msg__Time__get_individual_type_description_source(NULL);
    sources[2] = *std_msgs__msg__Header__get_individual_type_description_source(NULL);
    constructed = true;
  }
  return &source_sequence;
}
