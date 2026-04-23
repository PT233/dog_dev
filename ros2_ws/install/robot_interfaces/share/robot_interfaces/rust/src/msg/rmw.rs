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


// This struct is not documented.
#[allow(missing_docs)]

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct TargetInfo {

    // This member is not documented.
    #[allow(missing_docs)]
    pub header: std_msgs::msg::rmw::Header,


    // This member is not documented.
    #[allow(missing_docs)]
    pub class_name: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub track_id: rosidl_runtime_rs::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub confidence: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub bbox_cx: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub bbox_cy: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub bbox_w: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub bbox_h: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub locked: bool,

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


