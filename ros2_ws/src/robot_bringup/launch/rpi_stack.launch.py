from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    # 树莓派侧只需要启动 UART 桥接，负责和 STM32 通过 /dev/ttyAMA0 通信。
    uart_bridge_dir = get_package_share_directory('uart_bridge')

    # 复用 uart_bridge 包内 launch，保持串口参数来源一致。
    uart_bridge_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(uart_bridge_dir, 'launch', 'uart_bridge.launch.py')))

    return LaunchDescription([
        uart_bridge_launch,
    ])
