from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    # Get package share directories
    uart_bridge_dir = get_package_share_directory('uart_bridge')

    # Launch files
    uart_bridge_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(uart_bridge_dir, 'launch', 'uart_bridge.launch.py')))

    return LaunchDescription([
        uart_bridge_launch,
    ])
