import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    config_dir = os.path.join(
        get_package_share_directory('uart_bridge'), 'config')
    config_file = os.path.join(config_dir, 'uart_bridge.yaml')

    uart_bridge_node = Node(
        package='uart_bridge',
        executable='uart_bridge_node',
        name='uart_bridge_node',
        parameters=[config_file],
        output='screen'
    )

    return LaunchDescription([uart_bridge_node])
