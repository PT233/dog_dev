#!/usr/bin/env python3

"""
模拟 uart_bridge 节点 - 用于测试环境.

功能：
1. 订阅私有 servo_command 输入（来自 leg_motion_node）
2. 发布私有 servo_state 输出（模拟舵机反馈）
3. 无需真实 UART 连接

用途：快速进行端到端视觉引导腿部控制测试，而无需树莓派上的完整 ROS 代码
"""

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy
from sensor_msgs.msg import JointState


class MockUartBridge(Node):
    def __init__(self):
        super().__init__('mock_uart_bridge_node')

        self.get_logger().info('Mock UART Bridge Node started')

        # 订阅舵机命令
        qos_profile = QoSProfile(
            reliability=ReliabilityPolicy.RELIABLE,
            depth=10
        )

        self._servo_command_subscription = self.create_subscription(
            JointState,
            '~/input/servo_command',
            self.servo_command_callback,
            qos_profile
        )

        # 发布舵机状态（模拟反馈）
        self._servo_state_publisher = self.create_publisher(
            JointState,
            '~/output/servo_state',
            qos_profile
        )

        # 模拟 4 路腿舵机状态
        self._servo_angles = [90.0, 90.0, 90.0, 90.0]

        # 定时发布状态（50Hz）
        self._timer = self.create_timer(0.02, self.publish_servo_state)

        self.get_logger().info('Mock UART Bridge initialized')
        self.get_logger().info('Subscribed to private servo_command input')
        self.get_logger().info('Publishing private servo_state output')

    def servo_command_callback(self, servo_command_msg: JointState):
        """
        接收腿舵机命令并模拟执行.

        servo_command_msg.name: ['front_left', 'front_right', 'rear_left', 'rear_right']
        servo_command_msg.position: [角度(弧度), ...]
        servo_command_msg.effort: [持续时间(ms), ...]
        """
        try:
            # 角度单位转换：弧度 → 度
            for servo_index, angle_radians in enumerate(servo_command_msg.position):
                angle_degrees = angle_radians * 180.0 / 3.14159265

                # 模拟舵机响应（平滑移动）
                current_angle = self._servo_angles[servo_index]
                target_angle = angle_degrees

                # 简单的平滑移动（实际舵机会有速度限制）
                angle_step = (target_angle - current_angle) * 0.2  # 20% 步长
                self._servo_angles[servo_index] = current_angle + angle_step

                self.get_logger().debug(
                    f'  Servo {servo_command_msg.name[servo_index]}: '
                    f'{current_angle:.1f}° → {target_angle:.1f}°'
                )
        except Exception as error:
            self.get_logger().warn(f'Error processing servo command: {error}')

    def publish_servo_state(self):
        """定期发布舵机状态."""
        servo_state_msg = JointState()
        servo_state_msg.header.stamp = self.get_clock().now().to_msg()
        servo_state_msg.name = ['front_left', 'front_right', 'rear_left', 'rear_right']

        # 角度单位转换：度 → 弧度
        servo_state_msg.position = [
            angle * 3.14159265 / 180.0
            for angle in self._servo_angles
        ]

        # 模拟速度和电流（0 表示空闲）
        servo_state_msg.velocity = [0.0] * 4
        servo_state_msg.effort = [0.0] * 4

        self._servo_state_publisher.publish(servo_state_msg)


def main(args=None):
    rclpy.init(args=args)

    node = MockUartBridge()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        print('\n[Mock UART Bridge] Shutting down...')
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
