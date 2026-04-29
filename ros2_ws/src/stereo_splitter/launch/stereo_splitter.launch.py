from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    # 单独启动双目裁剪节点，取左半幅并发布 /camera/image_mono。
    stereo_splitter_node = Node(
        package='stereo_splitter',
        executable='stereo_splitter_node',
        name='stereo_splitter_node',
        output='screen'
    )

    return LaunchDescription([
        stereo_splitter_node
    ])
