#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};



// Corresponds to robot_interfaces__msg__TargetInfo

// This struct is not documented.
#[allow(missing_docs)]

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct TargetInfo {

    // This member is not documented.
    #[allow(missing_docs)]
    pub header: std_msgs::msg::Header,


    // This member is not documented.
    #[allow(missing_docs)]
    pub class_name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub track_id: std::string::String,


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
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::TargetInfo::default())
  }
}

impl rosidl_runtime_rs::Message for TargetInfo {
  type RmwMsg = super::msg::rmw::TargetInfo;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Owned(msg.header)).into_owned(),
        class_name: msg.class_name.as_str().into(),
        track_id: msg.track_id.as_str().into(),
        confidence: msg.confidence,
        bbox_cx: msg.bbox_cx,
        bbox_cy: msg.bbox_cy,
        bbox_w: msg.bbox_w,
        bbox_h: msg.bbox_h,
        locked: msg.locked,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Borrowed(&msg.header)).into_owned(),
        class_name: msg.class_name.as_str().into(),
        track_id: msg.track_id.as_str().into(),
      confidence: msg.confidence,
      bbox_cx: msg.bbox_cx,
      bbox_cy: msg.bbox_cy,
      bbox_w: msg.bbox_w,
      bbox_h: msg.bbox_h,
      locked: msg.locked,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      header: std_msgs::msg::Header::from_rmw_message(msg.header),
      class_name: msg.class_name.to_string(),
      track_id: msg.track_id.to_string(),
      confidence: msg.confidence,
      bbox_cx: msg.bbox_cx,
      bbox_cy: msg.bbox_cy,
      bbox_w: msg.bbox_w,
      bbox_h: msg.bbox_h,
      locked: msg.locked,
    }
  }
}


// Corresponds to robot_interfaces__msg__SimpleDetection

// This struct is not documented.
#[allow(missing_docs)]

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SimpleDetection {

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


    // This member is not documented.
    #[allow(missing_docs)]
    pub confidence: f32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub class_id: i32,


    // This member is not documented.
    #[allow(missing_docs)]
    pub track_id: std::string::String,

}



impl Default for SimpleDetection {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::SimpleDetection::default())
  }
}

impl rosidl_runtime_rs::Message for SimpleDetection {
  type RmwMsg = super::msg::rmw::SimpleDetection;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        center_x: msg.center_x,
        center_y: msg.center_y,
        width: msg.width,
        height: msg.height,
        confidence: msg.confidence,
        class_id: msg.class_id,
        track_id: msg.track_id.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      center_x: msg.center_x,
      center_y: msg.center_y,
      width: msg.width,
      height: msg.height,
      confidence: msg.confidence,
      class_id: msg.class_id,
        track_id: msg.track_id.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      center_x: msg.center_x,
      center_y: msg.center_y,
      width: msg.width,
      height: msg.height,
      confidence: msg.confidence,
      class_id: msg.class_id,
      track_id: msg.track_id.to_string(),
    }
  }
}


// Corresponds to robot_interfaces__msg__SimpleDetection2DArray

// This struct is not documented.
#[allow(missing_docs)]

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct SimpleDetection2DArray {

    // This member is not documented.
    #[allow(missing_docs)]
    pub header: std_msgs::msg::Header,


    // This member is not documented.
    #[allow(missing_docs)]
    pub detections: Vec<super::msg::SimpleDetection>,

}



impl Default for SimpleDetection2DArray {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::SimpleDetection2DArray::default())
  }
}

impl rosidl_runtime_rs::Message for SimpleDetection2DArray {
  type RmwMsg = super::msg::rmw::SimpleDetection2DArray;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Owned(msg.header)).into_owned(),
        detections: msg.detections
          .into_iter()
          .map(|elem| super::msg::SimpleDetection::into_rmw_message(std::borrow::Cow::Owned(elem)).into_owned())
          .collect(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Borrowed(&msg.header)).into_owned(),
        detections: msg.detections
          .iter()
          .map(|elem| super::msg::SimpleDetection::into_rmw_message(std::borrow::Cow::Borrowed(elem)).into_owned())
          .collect(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      header: std_msgs::msg::Header::from_rmw_message(msg.header),
      detections: msg.detections
          .into_iter()
          .map(super::msg::SimpleDetection::from_rmw_message)
          .collect(),
    }
  }
}


