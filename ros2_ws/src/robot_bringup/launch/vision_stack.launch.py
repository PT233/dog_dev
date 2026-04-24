from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    # Get package share directories
    gst_receiver_dir = get_package_share_directory('gst_receiver')
    stereo_splitter_dir = get_package_share_directory('stereo_splitter')
    detection_node_dir = get_package_share_directory('detection_node')
    tracker_node_dir = get_package_share_directory('tracker_node')
    behavior_node_dir = get_package_share_directory('behavior_node')
    visual_servo_dir = get_package_share_directory('visual_servo')

    # Launch files
    gst_receiver_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(gst_receiver_dir, 'launch', 'gst_receiver.launch.py')))

    stereo_splitter_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(stereo_splitter_dir, 'launch', 'stereo_splitter.launch.py')))

    detection_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(detection_node_dir, 'launch', 'detection.launch.py')))

    tracker_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(tracker_node_dir, 'launch', 'tracker.launch.py')))

    behavior_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(behavior_node_dir, 'launch', 'behavior.launch.py')))

    visual_servo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(visual_servo_dir, 'launch', 'visual_servo.launch.py')))

    return LaunchDescription([
        gst_receiver_launch,
        stereo_splitter_launch,
        detection_launch,
        tracker_launch,
        behavior_launch,
        visual_servo_launch,
    ])
