import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # tracker.yaml 跟随 package 安装，运行时从 share 目录解析路径。
    config_dir = os.path.join(
        get_package_share_directory('tracker_node'),
        'config'
    )

    ld = LaunchDescription([
        # tracker_node 将私有检测输入转成带 track_id 的私有跟踪输出。
        Node(
            package='tracker_node',
            executable='tracker_node_exe',
            name='tracker_node',
            output='screen',
            parameters=[os.path.join(config_dir, 'tracker.yaml')]
        )
    ])

    return ld
