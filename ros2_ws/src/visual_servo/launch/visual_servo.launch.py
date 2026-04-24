from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    visual_servo_dir = get_package_share_directory('visual_servo')
    config_dir = os.path.join(visual_servo_dir, '..', '..', '..', 'config')
    config_file = os.path.join(config_dir, 'visual_servo.yaml')

    visual_servo_node = Node(
        package='visual_servo',
        executable='visual_servo_node_exe',
        name='visual_servo_node',
        output='screen',
        parameters=[config_file]
    )

    return LaunchDescription([
        visual_servo_node
    ])
