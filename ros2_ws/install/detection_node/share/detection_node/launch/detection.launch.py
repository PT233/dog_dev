from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    detection_node_dir = get_package_share_directory('detection_node')
    config_file = os.path.join(detection_node_dir, 'config', 'detection.yaml')

    detection_node = Node(
        package='detection_node',
        executable='detection_node_exe',
        name='detection_node',
        parameters=[config_file],
        output='screen'
    )

    return LaunchDescription([
        detection_node,
    ])
