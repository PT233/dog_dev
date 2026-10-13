from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # 单独启动 IMU 节点：读 /dev/mpu6050_*，发布到私有 output topic。
    imu_node = Node(
        package='imu_node',
        executable='imu_node',
        name='imu_node',
        output='screen'
    )

    return LaunchDescription([
        imu_node
    ])
