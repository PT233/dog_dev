import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # 从安装后的 package share 目录读取检测参数文件。
    detection_node_dir = get_package_share_directory('detection_node')
    config_file = os.path.join(detection_node_dir, 'config', 'detection.yaml')

    # YOLO 检测节点：使用私有 input/output topic，跨节点连接由 bringup remap。
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
