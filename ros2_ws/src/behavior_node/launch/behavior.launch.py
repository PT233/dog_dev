from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    behavior_node_dir = get_package_share_directory('behavior_node')
    config_dir = os.path.join(behavior_node_dir, 'config')
    config_file = os.path.join(config_dir, 'behavior.yaml')

    behavior_node = Node(
        package='behavior_node',
        executable='behavior_node_exe',
        name='behavior_node',
        output='screen',
        parameters=[config_file]
    )

    return LaunchDescription([
        behavior_node
    ])
