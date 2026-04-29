from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    # 单独启动 UDP/H.264 接收节点，输出完整双目拼接图 /stereo/image_raw。
    gst_receiver_node = Node(
        package='gst_receiver',
        executable='gst_receiver_node',
        name='gst_receiver_node',
        output='screen'
    )

    return LaunchDescription([
        gst_receiver_node
    ])
