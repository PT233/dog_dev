# generated from rosidl_generator_py/resource/_idl.py.em
# with input from robot_interfaces:msg/TargetInfo.idl
# generated code does not contain a copyright notice

# This is being done at the module level and not on the instance level to avoid looking
# for the same variable multiple times on each instance. This variable is not supposed to
# change during runtime so it makes sense to only look for it once.
from os import getenv

ros_python_check_fields = getenv('ROS_PYTHON_CHECK_FIELDS', default='')


# Import statements for member types

import builtins  # noqa: E402, I100

import math  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_TargetInfo(type):
    """Metaclass of message 'TargetInfo'."""

    _CREATE_ROS_MESSAGE = None
    _CONVERT_FROM_PY = None
    _CONVERT_TO_PY = None
    _DESTROY_ROS_MESSAGE = None
    _TYPE_SUPPORT = None

    __constants = {
    }

    @classmethod
    def __import_type_support__(cls):
        try:
            from rosidl_generator_py import import_type_support
            module = import_type_support('robot_interfaces')
        except ImportError:
            import logging
            import traceback
            logger = logging.getLogger(
                'robot_interfaces.msg.TargetInfo')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__target_info
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__target_info
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__target_info
            cls._TYPE_SUPPORT = module.type_support_msg__msg__target_info
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__target_info

            from std_msgs.msg import Header
            if Header.__class__._TYPE_SUPPORT is None:
                Header.__class__.__import_type_support__()

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class TargetInfo(metaclass=Metaclass_TargetInfo):
    """Message class 'TargetInfo'."""

    __slots__ = [
        '_header',
        '_class_name',
        '_track_id',
        '_confidence',
        '_bbox_cx',
        '_bbox_cy',
        '_bbox_w',
        '_bbox_h',
        '_locked',
        '_check_fields',
    ]

    _fields_and_field_types = {
        'header': 'std_msgs/Header',
        'class_name': 'string',
        'track_id': 'string',
        'confidence': 'float',
        'bbox_cx': 'int32',
        'bbox_cy': 'int32',
        'bbox_w': 'int32',
        'bbox_h': 'int32',
        'locked': 'boolean',
    }

    # This attribute is used to store an rosidl_parser.definition variable
    # related to the data type of each of the components the message.
    SLOT_TYPES = (
        rosidl_parser.definition.NamespacedType(['std_msgs', 'msg'], 'Header'),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.UnboundedString(),  # noqa: E501
        rosidl_parser.definition.BasicType('float'),  # noqa: E501
        rosidl_parser.definition.BasicType('int32'),  # noqa: E501
        rosidl_parser.definition.BasicType('int32'),  # noqa: E501
        rosidl_parser.definition.BasicType('int32'),  # noqa: E501
        rosidl_parser.definition.BasicType('int32'),  # noqa: E501
        rosidl_parser.definition.BasicType('boolean'),  # noqa: E501
    )

    def __init__(self, **kwargs):
        if 'check_fields' in kwargs:
            self._check_fields = kwargs['check_fields']
        else:
            self._check_fields = ros_python_check_fields == '1'
        if self._check_fields:
            assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
                'Invalid arguments passed to constructor: %s' % \
                ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        from std_msgs.msg import Header
        self.header = kwargs.get('header', Header())
        self.class_name = kwargs.get('class_name', str())
        self.track_id = kwargs.get('track_id', str())
        self.confidence = kwargs.get('confidence', float())
        self.bbox_cx = kwargs.get('bbox_cx', int())
        self.bbox_cy = kwargs.get('bbox_cy', int())
        self.bbox_w = kwargs.get('bbox_w', int())
        self.bbox_h = kwargs.get('bbox_h', int())
        self.locked = kwargs.get('locked', bool())

    def __repr__(self):
        typename = self.__class__.__module__.split('.')
        typename.pop()
        typename.append(self.__class__.__name__)
        args = []
        for s, t in zip(self.get_fields_and_field_types().keys(), self.SLOT_TYPES):
            field = getattr(self, s)
            fieldstr = repr(field)
            # We use Python array type for fields that can be directly stored
            # in them, and "normal" sequences for everything else.  If it is
            # a type that we store in an array, strip off the 'array' portion.
            if (
                isinstance(t, rosidl_parser.definition.AbstractSequence) and
                isinstance(t.value_type, rosidl_parser.definition.BasicType) and
                t.value_type.typename in ['float', 'double', 'int8', 'uint8', 'int16', 'uint16', 'int32', 'uint32', 'int64', 'uint64']
            ):
                if len(field) == 0:
                    fieldstr = '[]'
                else:
                    if self._check_fields:
                        assert fieldstr.startswith('array(')
                    prefix = "array('X', "
                    suffix = ')'
                    fieldstr = fieldstr[len(prefix):-len(suffix)]
            args.append(s + '=' + fieldstr)
        return '%s(%s)' % ('.'.join(typename), ', '.join(args))

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            return False
        if self.header != other.header:
            return False
        if self.class_name != other.class_name:
            return False
        if self.track_id != other.track_id:
            return False
        if self.confidence != other.confidence:
            return False
        if self.bbox_cx != other.bbox_cx:
            return False
        if self.bbox_cy != other.bbox_cy:
            return False
        if self.bbox_w != other.bbox_w:
            return False
        if self.bbox_h != other.bbox_h:
            return False
        if self.locked != other.locked:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def header(self):
        """Message field 'header'."""
        return self._header

    @header.setter
    def header(self, value):
        if self._check_fields:
            from std_msgs.msg import Header
            assert \
                isinstance(value, Header), \
                "The 'header' field must be a sub message of type 'Header'"
        self._header = value

    @builtins.property
    def class_name(self):
        """Message field 'class_name'."""
        return self._class_name

    @class_name.setter
    def class_name(self, value):
        if self._check_fields:
            assert \
                isinstance(value, str), \
                "The 'class_name' field must be of type 'str'"
        self._class_name = value

    @builtins.property
    def track_id(self):
        """Message field 'track_id'."""
        return self._track_id

    @track_id.setter
    def track_id(self, value):
        if self._check_fields:
            assert \
                isinstance(value, str), \
                "The 'track_id' field must be of type 'str'"
        self._track_id = value

    @builtins.property
    def confidence(self):
        """Message field 'confidence'."""
        return self._confidence

    @confidence.setter
    def confidence(self, value):
        if self._check_fields:
            assert \
                isinstance(value, float), \
                "The 'confidence' field must be of type 'float'"
            assert not (value < -3.402823466e+38 or value > 3.402823466e+38) or math.isinf(value), \
                "The 'confidence' field must be a float in [-3.402823466e+38, 3.402823466e+38]"
        self._confidence = value

    @builtins.property
    def bbox_cx(self):
        """Message field 'bbox_cx'."""
        return self._bbox_cx

    @bbox_cx.setter
    def bbox_cx(self, value):
        if self._check_fields:
            assert \
                isinstance(value, int), \
                "The 'bbox_cx' field must be of type 'int'"
            assert value >= -2147483648 and value < 2147483648, \
                "The 'bbox_cx' field must be an integer in [-2147483648, 2147483647]"
        self._bbox_cx = value

    @builtins.property
    def bbox_cy(self):
        """Message field 'bbox_cy'."""
        return self._bbox_cy

    @bbox_cy.setter
    def bbox_cy(self, value):
        if self._check_fields:
            assert \
                isinstance(value, int), \
                "The 'bbox_cy' field must be of type 'int'"
            assert value >= -2147483648 and value < 2147483648, \
                "The 'bbox_cy' field must be an integer in [-2147483648, 2147483647]"
        self._bbox_cy = value

    @builtins.property
    def bbox_w(self):
        """Message field 'bbox_w'."""
        return self._bbox_w

    @bbox_w.setter
    def bbox_w(self, value):
        if self._check_fields:
            assert \
                isinstance(value, int), \
                "The 'bbox_w' field must be of type 'int'"
            assert value >= -2147483648 and value < 2147483648, \
                "The 'bbox_w' field must be an integer in [-2147483648, 2147483647]"
        self._bbox_w = value

    @builtins.property
    def bbox_h(self):
        """Message field 'bbox_h'."""
        return self._bbox_h

    @bbox_h.setter
    def bbox_h(self, value):
        if self._check_fields:
            assert \
                isinstance(value, int), \
                "The 'bbox_h' field must be of type 'int'"
            assert value >= -2147483648 and value < 2147483648, \
                "The 'bbox_h' field must be an integer in [-2147483648, 2147483647]"
        self._bbox_h = value

    @builtins.property
    def locked(self):
        """Message field 'locked'."""
        return self._locked

    @locked.setter
    def locked(self, value):
        if self._check_fields:
            assert \
                isinstance(value, bool), \
                "The 'locked' field must be of type 'bool'"
        self._locked = value
