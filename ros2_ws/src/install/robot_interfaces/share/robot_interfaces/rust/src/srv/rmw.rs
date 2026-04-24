#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};



#[link(name = "robot_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__srv__SetTargetClass_Request() -> *const std::ffi::c_void;
}

#[link(name = "robot_interfaces__rosidl_generator_c")]
extern "C" {
    fn robot_interfaces__srv__SetTargetClass_Request__init(msg: *mut SetTargetClass_Request) -> bool;
    fn robot_interfaces__srv__SetTargetClass_Request__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<SetTargetClass_Request>, size: usize) -> bool;
    fn robot_interfaces__srv__SetTargetClass_Request__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<SetTargetClass_Request>);
    fn robot_interfaces__srv__SetTargetClass_Request__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<SetTargetClass_Request>, out_seq: *mut rosidl_runtime_rs::Sequence<SetTargetClass_Request>) -> bool;
}

// Corresponds to robot_interfaces__srv__SetTargetClass_Request
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]


// This struct is not documented.
#[allow(missing_docs)]

#[allow(non_camel_case_types)]
#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetTargetClass_Request {

    // This member is not documented.
    #[allow(missing_docs)]
    pub class_name: rosidl_runtime_rs::String,

}



impl Default for SetTargetClass_Request {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !robot_interfaces__srv__SetTargetClass_Request__init(&mut msg as *mut _) {
        panic!("Call to robot_interfaces__srv__SetTargetClass_Request__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for SetTargetClass_Request {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__srv__SetTargetClass_Request__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__srv__SetTargetClass_Request__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__srv__SetTargetClass_Request__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for SetTargetClass_Request {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for SetTargetClass_Request where Self: Sized {
  const TYPE_NAME: &'static str = "robot_interfaces/srv/SetTargetClass_Request";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__srv__SetTargetClass_Request() }
  }
}


#[link(name = "robot_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__srv__SetTargetClass_Response() -> *const std::ffi::c_void;
}

#[link(name = "robot_interfaces__rosidl_generator_c")]
extern "C" {
    fn robot_interfaces__srv__SetTargetClass_Response__init(msg: *mut SetTargetClass_Response) -> bool;
    fn robot_interfaces__srv__SetTargetClass_Response__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<SetTargetClass_Response>, size: usize) -> bool;
    fn robot_interfaces__srv__SetTargetClass_Response__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<SetTargetClass_Response>);
    fn robot_interfaces__srv__SetTargetClass_Response__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<SetTargetClass_Response>, out_seq: *mut rosidl_runtime_rs::Sequence<SetTargetClass_Response>) -> bool;
}

// Corresponds to robot_interfaces__srv__SetTargetClass_Response
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]


// This struct is not documented.
#[allow(missing_docs)]

#[allow(non_camel_case_types)]
#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SetTargetClass_Response {

    // This member is not documented.
    #[allow(missing_docs)]
    pub success: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub message: rosidl_runtime_rs::String,

}



impl Default for SetTargetClass_Response {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !robot_interfaces__srv__SetTargetClass_Response__init(&mut msg as *mut _) {
        panic!("Call to robot_interfaces__srv__SetTargetClass_Response__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for SetTargetClass_Response {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__srv__SetTargetClass_Response__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__srv__SetTargetClass_Response__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__srv__SetTargetClass_Response__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for SetTargetClass_Response {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for SetTargetClass_Response where Self: Sized {
  const TYPE_NAME: &'static str = "robot_interfaces/srv/SetTargetClass_Response";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__srv__SetTargetClass_Response() }
  }
}


#[link(name = "robot_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__srv__CalibrateCenter_Request() -> *const std::ffi::c_void;
}

#[link(name = "robot_interfaces__rosidl_generator_c")]
extern "C" {
    fn robot_interfaces__srv__CalibrateCenter_Request__init(msg: *mut CalibrateCenter_Request) -> bool;
    fn robot_interfaces__srv__CalibrateCenter_Request__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<CalibrateCenter_Request>, size: usize) -> bool;
    fn robot_interfaces__srv__CalibrateCenter_Request__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<CalibrateCenter_Request>);
    fn robot_interfaces__srv__CalibrateCenter_Request__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<CalibrateCenter_Request>, out_seq: *mut rosidl_runtime_rs::Sequence<CalibrateCenter_Request>) -> bool;
}

// Corresponds to robot_interfaces__srv__CalibrateCenter_Request
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]


// This struct is not documented.
#[allow(missing_docs)]

#[allow(non_camel_case_types)]
#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct CalibrateCenter_Request {

    // This member is not documented.
    #[allow(missing_docs)]
    pub center_x: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub center_y: i32,

}



impl Default for CalibrateCenter_Request {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !robot_interfaces__srv__CalibrateCenter_Request__init(&mut msg as *mut _) {
        panic!("Call to robot_interfaces__srv__CalibrateCenter_Request__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for CalibrateCenter_Request {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__srv__CalibrateCenter_Request__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__srv__CalibrateCenter_Request__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__srv__CalibrateCenter_Request__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for CalibrateCenter_Request {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for CalibrateCenter_Request where Self: Sized {
  const TYPE_NAME: &'static str = "robot_interfaces/srv/CalibrateCenter_Request";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__srv__CalibrateCenter_Request() }
  }
}


#[link(name = "robot_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__srv__CalibrateCenter_Response() -> *const std::ffi::c_void;
}

#[link(name = "robot_interfaces__rosidl_generator_c")]
extern "C" {
    fn robot_interfaces__srv__CalibrateCenter_Response__init(msg: *mut CalibrateCenter_Response) -> bool;
    fn robot_interfaces__srv__CalibrateCenter_Response__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<CalibrateCenter_Response>, size: usize) -> bool;
    fn robot_interfaces__srv__CalibrateCenter_Response__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<CalibrateCenter_Response>);
    fn robot_interfaces__srv__CalibrateCenter_Response__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<CalibrateCenter_Response>, out_seq: *mut rosidl_runtime_rs::Sequence<CalibrateCenter_Response>) -> bool;
}

// Corresponds to robot_interfaces__srv__CalibrateCenter_Response
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]


// This struct is not documented.
#[allow(missing_docs)]

#[allow(non_camel_case_types)]
#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct CalibrateCenter_Response {

    // This member is not documented.
    #[allow(missing_docs)]
    pub success: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub message: rosidl_runtime_rs::String,

}



impl Default for CalibrateCenter_Response {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !robot_interfaces__srv__CalibrateCenter_Response__init(&mut msg as *mut _) {
        panic!("Call to robot_interfaces__srv__CalibrateCenter_Response__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for CalibrateCenter_Response {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__srv__CalibrateCenter_Response__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__srv__CalibrateCenter_Response__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { robot_interfaces__srv__CalibrateCenter_Response__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for CalibrateCenter_Response {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for CalibrateCenter_Response where Self: Sized {
  const TYPE_NAME: &'static str = "robot_interfaces/srv/CalibrateCenter_Response";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__robot_interfaces__srv__CalibrateCenter_Response() }
  }
}






#[link(name = "robot_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_service_type_support_handle__robot_interfaces__srv__SetTargetClass() -> *const std::ffi::c_void;
}

// Corresponds to robot_interfaces__srv__SetTargetClass
#[allow(missing_docs, non_camel_case_types)]
pub struct SetTargetClass;

impl rosidl_runtime_rs::Service for SetTargetClass {
    type Request = SetTargetClass_Request;
    type Response = SetTargetClass_Response;

    fn get_type_support() -> *const std::ffi::c_void {
        // SAFETY: No preconditions for this function.
        unsafe { rosidl_typesupport_c__get_service_type_support_handle__robot_interfaces__srv__SetTargetClass() }
    }
}




#[link(name = "robot_interfaces__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_service_type_support_handle__robot_interfaces__srv__CalibrateCenter() -> *const std::ffi::c_void;
}

// Corresponds to robot_interfaces__srv__CalibrateCenter
#[allow(missing_docs, non_camel_case_types)]
pub struct CalibrateCenter;

impl rosidl_runtime_rs::Service for CalibrateCenter {
    type Request = CalibrateCenter_Request;
    type Response = CalibrateCenter_Response;

    fn get_type_support() -> *const std::ffi::c_void {
        // SAFETY: No preconditions for this function.
        unsafe { rosidl_typesupport_c__get_service_type_support_handle__robot_interfaces__srv__CalibrateCenter() }
    }
}


