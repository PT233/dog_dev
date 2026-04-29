"""
任务 7.3 完整启动文件 - 包含所有节点 + 模拟 uart_bridge

用于快速测试场景：
- 树莓派上只运行相机推流
- WSL2 上运行所有视觉节点 + 模拟 uart_bridge
"""

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    # 集成测试启动：视觉链路真实运行，UART 桥用 mock 节点替代硬件。
    gst_receiver_dir = get_package_share_directory('gst_receiver')
    stereo_splitter_dir = get_package_share_directory('stereo_splitter')
    detection_node_dir = get_package_share_directory('detection_node')
    tracker_node_dir = get_package_share_directory('tracker_node')
    behavior_node_dir = get_package_share_directory('behavior_node')
    visual_servo_dir = get_package_share_directory('visual_servo')

    # 视觉流水线节点，顺序对应图像流和控制流的处理阶段。
    gst_receiver_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(gst_receiver_dir, 'launch', 'gst_receiver.launch.py')))

    stereo_splitter_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(stereo_splitter_dir, 'launch', 'stereo_splitter.launch.py')))

    detection_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(detection_node_dir, 'launch', 'detection.launch.py')))

    tracker_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(tracker_node_dir, 'launch', 'tracker.launch.py')))

    behavior_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(behavior_node_dir, 'launch', 'behavior.launch.py')))

    visual_servo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(visual_servo_dir, 'launch', 'visual_servo.launch.py')))

    # mock UART bridge 模拟树莓派 uart_bridge，便于无 STM32 硬件时验证 ROS 侧闭环。
    mock_uart_bridge = Node(
        package='robot_bringup',
        executable='mock_uart_bridge',
        name='mock_uart_bridge_node',
        output='screen',
    )

    return LaunchDescription([
        gst_receiver_launch,
        stereo_splitter_launch,
        detection_launch,
        tracker_launch,
        behavior_launch,
        visual_servo_launch,
        mock_uart_bridge,  # 加入模拟 UART 桥接节点
    ])
