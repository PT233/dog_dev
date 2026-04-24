from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration
from launch.actions import DeclareLaunchArgument
import os
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    config_dir = os.path.join(
        get_package_share_directory('tracker_node'),
        'config'
    )

    ld = LaunchDescription([
        Node(
            package='tracker_node',
            executable='tracker_node_exe',
            name='tracker_node',
            output='screen',
            parameters=[os.path.join(config_dir, 'tracker.yaml')]
        )
    ])

    return ld
