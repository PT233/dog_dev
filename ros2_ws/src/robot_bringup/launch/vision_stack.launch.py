import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # 视觉闭环完整链路：接收相机流、裁剪、检测、跟踪、目标选择、生成舵机命令。
    detection_node_dir = get_package_share_directory('detection_node')
    tracker_node_dir = get_package_share_directory('tracker_node')
    behavior_node_dir = get_package_share_directory('behavior_node')
    visual_servo_dir = get_package_share_directory('visual_servo')

    detection_config = os.path.join(detection_node_dir, 'config', 'detection.yaml')
    tracker_config = os.path.join(tracker_node_dir, 'config', 'tracker.yaml')
    behavior_config = os.path.join(behavior_node_dir, 'config', 'behavior.yaml')
    visual_servo_config = os.path.join(visual_servo_dir, 'config', 'visual_servo.yaml')

    return LaunchDescription([
        Node(
            package='gst_receiver',
            executable='gst_receiver_node',
            name='gst_receiver_node',
            output='screen',
        ),
        Node(
            package='stereo_splitter',
            executable='stereo_splitter_node',
            name='stereo_splitter_node',
            output='screen',
            remappings=[
                ('~/input/stereo_image_raw',
                 'gst_receiver_node/output/stereo_image_raw'),
            ],
        ),
        Node(
            package='detection_node',
            executable='detection_node_exe',
            name='detection_node',
            output='screen',
            parameters=[detection_config],
            remappings=[
                ('~/input/image', 'stereo_splitter_node/output/left_image'),
            ],
        ),
        Node(
            package='tracker_node',
            executable='tracker_node_exe',
            name='tracker_node',
            output='screen',
            parameters=[tracker_config],
            remappings=[
                ('~/input/detections', 'detection_node/output/detections'),
            ],
        ),
        Node(
            package='behavior_node',
            executable='behavior_node_exe',
            name='behavior_node',
            output='screen',
            parameters=[behavior_config],
            remappings=[
                ('~/input/tracked_objects', 'tracker_node/output/tracked_objects'),
            ],
        ),
        Node(
            package='visual_servo',
            executable='visual_servo_node_exe',
            name='leg_motion_node',
            output='screen',
            parameters=[visual_servo_config],
            remappings=[
                ('~/input/pixel_error', 'behavior_node/output/pixel_error'),
                ('~/input/servo_state', 'uart_bridge_node/output/servo_state'),
            ],
        ),
    ])
