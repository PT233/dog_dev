import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # 视觉伺服参数控制 PID、步态幅度、舵机限位和目标超时。
    visual_servo_dir = get_package_share_directory('visual_servo')
    config_dir = os.path.join(visual_servo_dir, 'config')
    config_file = os.path.join(config_dir, 'visual_servo.yaml')

    visual_servo_node = Node(
        package='visual_servo',
        executable='visual_servo_node_exe',
        name='leg_motion_node',
        output='screen',
        parameters=[config_file]
    )

    return LaunchDescription([
        visual_servo_node
    ])
