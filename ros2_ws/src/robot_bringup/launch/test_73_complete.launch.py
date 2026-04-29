"""
任务 7.3 完整启动文件 - 包含所有节点 + 模拟 uart_bridge.

用于快速测试场景：
- 树莓派上只运行相机推流
- WSL2 上运行所有视觉节点 + 模拟 uart_bridge
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # 集成测试启动：视觉链路真实运行，UART 桥用 mock 节点替代硬件。
    detection_node_dir = get_package_share_directory('detection_node')
    tracker_node_dir = get_package_share_directory('tracker_node')
    behavior_node_dir = get_package_share_directory('behavior_node')
    visual_servo_dir = get_package_share_directory('visual_servo')

    detection_config = os.path.join(detection_node_dir, 'config', 'detection.yaml')
    tracker_config = os.path.join(tracker_node_dir, 'config', 'tracker.yaml')
    behavior_config = os.path.join(behavior_node_dir, 'config', 'behavior.yaml')
    visual_servo_config = os.path.join(visual_servo_dir, 'config', 'visual_servo.yaml')

    # mock UART bridge 模拟树莓派 uart_bridge，便于无 STM32 硬件时验证 ROS 侧闭环。
    mock_uart_bridge = Node(
        package='robot_bringup',
        executable='mock_uart_bridge',
        name='mock_uart_bridge_node',
        output='screen',
        remappings=[
            ('~/input/servo_command', 'leg_motion_node/output/servo_command'),
        ],
    )

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
                ('~/input/servo_state', 'mock_uart_bridge_node/output/servo_state'),
            ],
        ),
        mock_uart_bridge,
    ])
