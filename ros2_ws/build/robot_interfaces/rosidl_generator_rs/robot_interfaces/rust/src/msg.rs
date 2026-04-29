#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};



// Corresponds to robot_interfaces__msg__TargetInfo
/// 当前目标选择结果，预留给更完整的行为/状态显示。

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct TargetInfo {

    // This member is not documented.
    #[allow(missing_docs)]
    pub header: std_msgs::msg::Header,

    /// 目标类别名和跟踪 ID。
    pub class_name: std::string::String,


    // This member is not documented.
    #[allow(missing_docs)]
    pub track_id: std::string::String,

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
        bounding_box_center_x: msg.bounding_box_center_x,
        bounding_box_center_y: msg.bounding_box_center_y,
        bounding_box_width: msg.bounding_box_width,
        bounding_box_height: msg.bounding_box_height,
        is_locked: msg.is_locked,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Borrowed(&msg.header)).into_owned(),
        class_name: msg.class_name.as_str().into(),
        track_id: msg.track_id.as_str().into(),
      confidence: msg.confidence,
      bounding_box_center_x: msg.bounding_box_center_x,
      bounding_box_center_y: msg.bounding_box_center_y,
      bounding_box_width: msg.bounding_box_width,
      bounding_box_height: msg.bounding_box_height,
      is_locked: msg.is_locked,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      header: std_msgs::msg::Header::from_rmw_message(msg.header),
      class_name: msg.class_name.to_string(),
      track_id: msg.track_id.to_string(),
      confidence: msg.confidence,
      bounding_box_center_x: msg.bounding_box_center_x,
      bounding_box_center_y: msg.bounding_box_center_y,
      bounding_box_width: msg.bounding_box_width,
      bounding_box_height: msg.bounding_box_height,
      is_locked: msg.is_locked,
    }
  }
}


// Corresponds to robot_interfaces__msg__Detection2D
/// 单个 2D 检测框。
/// 坐标采用图像像素坐标，center_x/center_y 是检测框中心点，width/height 是框尺寸。

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
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
    pub track_id: std::string::String,

}



impl Default for Detection2D {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::Detection2D::default())
  }
}

impl rosidl_runtime_rs::Message for Detection2D {
  type RmwMsg = super::msg::rmw::Detection2D;

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


// Corresponds to robot_interfaces__msg__Detection2DArray
/// 一帧检测/跟踪结果。
/// header.stamp 应沿用原始图像时间戳，便于可视化节点找到对应画面。

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct Detection2DArray {

    // This member is not documented.
    #[allow(missing_docs)]
    pub header: std_msgs::msg::Header,

    /// 同一帧内的所有检测框。
    pub detections: Vec<super::msg::Detection2D>,

}



impl Default for Detection2DArray {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::Detection2DArray::default())
  }
}

impl rosidl_runtime_rs::Message for Detection2DArray {
  type RmwMsg = super::msg::rmw::Detection2DArray;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Owned(msg.header)).into_owned(),
        detections: msg.detections
          .into_iter()
          .map(|elem| super::msg::Detection2D::into_rmw_message(std::borrow::Cow::Owned(elem)).into_owned())
          .collect(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        header: std_msgs::msg::Header::into_rmw_message(std::borrow::Cow::Borrowed(&msg.header)).into_owned(),
        detections: msg.detections
          .iter()
          .map(|elem| super::msg::Detection2D::into_rmw_message(std::borrow::Cow::Borrowed(elem)).into_owned())
          .collect(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      header: std_msgs::msg::Header::from_rmw_message(msg.header),
      detections: msg.detections
          .into_iter()
          .map(super::msg::Detection2D::from_rmw_message)
          .collect(),
    }
  }
}


