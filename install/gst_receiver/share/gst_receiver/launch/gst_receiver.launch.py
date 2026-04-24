from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    gst_receiver_node = Node(
        package='gst_receiver',
        executable='gst_receiver_node',
        name='gst_receiver_node',
        output='screen'
    )

    return LaunchDescription([
        gst_receiver_node
    ])
