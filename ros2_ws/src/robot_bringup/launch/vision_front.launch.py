from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # 单进程视觉前端：gst_receiver + stereo_splitter + detection_node 组合在一个可执行文件中。
    return LaunchDescription([
        Node(
            package='robot_bringup',
            executable='vision_front',
            output='screen',
            emulate_tty=True,
        ),
    ])
