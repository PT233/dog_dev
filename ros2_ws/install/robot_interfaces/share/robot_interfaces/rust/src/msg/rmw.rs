#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};


#[link(name = "robot_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__msg__TargetInfo() -> *const std::ffi::c_void;
}

#[link(name = "robot_interfaces__rosidl_generator_c")]
extern "C" {
    fn robot_interfaces__msg__TargetInfo__init(msg: *mut TargetInfo) -> bool;
    fn robot_interfaces__msg__TargetInfo__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<TargetInfo>, size: usize) -> bool;
    fn robot_interfaces__msg__TargetInfo__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<TargetInfo>);
    fn robot_interfaces__msg__TargetInfo__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<TargetInfo>, out_seq: *mut rosidl_runtime_rs::Sequence<TargetInfo>) -> bool;
}

// Corresponds to robot_interfaces__msg__TargetInfo
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// 当前目标选择结果，预留给更完整的行为/状态显示。

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct TargetInfo {

    // This member is not documented.
    #[allow(missing_docs)]
    pub header: std_msgs::msg::rmw::Header,

    /// 目标类别名和跟踪 ID。
    pub class_name: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub track_id: rosidl_runtime_rs::String,

    /// 当前目标置信度和检测框像素位置。
    pub confidence: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub bounding_box_center_x: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub bounding_box_center_y: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub bounding_box_width: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub bounding_box_height: i32,

    /// true 表示当前行为节点已经锁定一个有效目标。
    pub is_locked: bool,

}



impl Default for TargetInfo {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !robot_interfaces__msg__TargetInfo__init(&mut msg as *mut _) {
        panic!("Call to robot_interfaces__msg__TargetInfo__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for TargetInfo {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__msg__TargetInfo__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__msg__TargetInfo__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__msg__TargetInfo__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for TargetInfo {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for TargetInfo where Self: Sized {
  const TYPE_NAME: &'static str = "robot_interfaces/msg/TargetInfo";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__msg__TargetInfo() }
  }
}


#[link(name = "robot_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__msg__Detection2D() -> *const std::ffi::c_void;
}

#[link(name = "robot_interfaces__rosidl_generator_c")]
extern "C" {
    fn robot_interfaces__msg__Detection2D__init(msg: *mut Detection2D) -> bool;
    fn robot_interfaces__msg__Detection2D__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<Detection2D>, size: usize) -> bool;
    fn robot_interfaces__msg__Detection2D__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<Detection2D>);
    fn robot_interfaces__msg__Detection2D__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<Detection2D>, out_seq: *mut rosidl_runtime_rs::Sequence<Detection2D>) -> bool;
}

// Corresponds to robot_interfaces__msg__Detection2D
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// 单个 2D 检测框。
/// 坐标采用图像像素坐标，center_x/center_y 是检测框中心点，width/height 是框尺寸。

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct Detection2D {

    // This member is not documented.
    #[allow(missing_docs)]
    pub center_x: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub center_y: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub width: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub height: f32,

    /// YOLO 输出的最终置信度，通常为 objectness × class_probability。
    pub confidence: f32,

    /// COCO 类别 ID；类别名由各节点通过 models/coco_classes.txt 映射。
    pub class_id: i32,

    /// tracker_node 填写的轨迹 ID；检测节点原始输出时为空字符串。
    pub track_id: rosidl_runtime_rs::String,

}



impl Default for Detection2D {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !robot_interfaces__msg__Detection2D__init(&mut msg as *mut _) {
        panic!("Call to robot_interfaces__msg__Detection2D__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for Detection2D {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__msg__Detection2D__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__msg__Detection2D__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__msg__Detection2D__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for Detection2D {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for Detection2D where Self: Sized {
  const TYPE_NAME: &'static str = "robot_interfaces/msg/Detection2D";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__msg__Detection2D() }
  }
}


#[link(name = "robot_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__msg__Detection2DArray() -> *const std::ffi::c_void;
}

#[link(name = "robot_interfaces__rosidl_generator_c")]
extern "C" {
    fn robot_interfaces__msg__Detection2DArray__init(msg: *mut Detection2DArray) -> bool;
    fn robot_interfaces__msg__Detection2DArray__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<Detection2DArray>, size: usize) -> bool;
    fn robot_interfaces__msg__Detection2DArray__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<Detection2DArray>);
    fn robot_interfaces__msg__Detection2DArray__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<Detection2DArray>, out_seq: *mut rosidl_runtime_rs::Sequence<Detection2DArray>) -> bool;
}

// Corresponds to robot_interfaces__msg__Detection2DArray
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// 一帧检测/跟踪结果。
/// header.stamp 应沿用原始图像时间戳，便于可视化节点找到对应画面。

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct Detection2DArray {

    // This member is not documented.
    #[allow(missing_docs)]
    pub header: std_msgs::msg::rmw::Header,

    /// 同一帧内的所有检测框。
    pub detections: rosidl_runtime_rs::Sequence<super::super::msg::rmw::Detection2D>,

}



impl Default for Detection2DArray {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !robot_interfaces__msg__Detection2DArray__init(&mut msg as *mut _) {
        panic!("Call to robot_interfaces__msg__Detection2DArray__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for Detection2DArray {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__msg__Detection2DArray__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__msg__Detection2DArray__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__msg__Detection2DArray__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for Detection2DArray {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for Detection2DArray where Self: Sized {
  const TYPE_NAME: &'static str = "robot_interfaces/msg/Detection2DArray";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__msg__Detection2DArray() }
  }
}


