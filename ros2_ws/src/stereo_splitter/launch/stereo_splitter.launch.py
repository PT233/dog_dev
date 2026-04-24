from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    stereo_splitter_node = Node(
        package='stereo_splitter',
        executable='stereo_splitter_node_exe',
        name='stereo_splitter_node',
        output='screen'
    )

    return LaunchDescription([
        stereo_splitter_node
    ])
