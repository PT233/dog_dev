from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='robot_bringup',
            executable='vision_front',
            output='screen',
            emulate_tty=True,
        ),
    ])
