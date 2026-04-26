#!/usr/bin/env python3

"""
模拟 uart_bridge 节点 - 用于测试环境

功能：
1. 订阅 /servo_cmd 话题（来自 visual_servo_node）
2. 发布 /servo_state 话题（模拟舵机反馈）
3. 无需真实 UART 连接

用途：快速进行端到端视觉追踪测试，而无需树莓派上的完整 ROS 代码
"""

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy
from sensor_msgs.msg import JointState
import time

class MockUARTBridge(Node):
    def __init__(self):
        super().__init__('mock_uart_bridge_node')

        self.get_logger().info('Mock UART Bridge Node started')

        # 订阅舵机命令
        qos_profile = QoSProfile(
            reliability=ReliabilityPolicy.RELIABLE,
            depth=10
        )

        self.servo_cmd_sub = self.create_subscription(
            JointState,
            '/servo_cmd',
            self.on_servo_cmd,
            qos_profile
        )

        # 发布舵机状态（模拟反馈）
        self.servo_state_pub = self.create_publisher(
            JointState,
            '/servo_state',
            qos_profile
        )

        # 模拟舵机状态
        self.servo_angles = [90.0, 90.0, 90.0, 90.0]  # 4个舵机的当前角度

        # 定时发布状态（50Hz）
        self.timer = self.create_timer(0.02, self.publish_servo_state)

        self.get_logger().info('Mock UART Bridge initialized')
        self.get_logger().info('Subscribed to /servo_cmd')
        self.get_logger().info('Publishing /servo_state')

    def on_servo_cmd(self, msg: JointState):
        """
        接收舵机命令并模拟执行

        msg.name: ['yaw', 'pitch', 's2', 's3']
        msg.position: [角度(弧度), ...]
        msg.effort: [持续时间(ms), ...]
        """
        try:
            # 角度单位转换：弧度 → 度
            for i, angle_rad in enumerate(msg.position):
                angle_deg = angle_rad * 180.0 / 3.14159265

                # 模拟舵机响应（平滑移动）
                current = self.servo_angles[i]
                target = angle_deg

                # 简单的平滑移动（实际舵机会有速度限制）
                step = (target - current) * 0.2  # 20% 步长
                self.servo_angles[i] = current + step

                if i < 2:  # 只打印前两个舵机（yaw, pitch）
                    self.get_logger().debug(
                        f'  Servo {msg.name[i]}: {current:.1f}° → {target:.1f}°'
                    )
        except Exception as e:
            self.get_logger().warn(f'Error processing servo command: {e}')

    def publish_servo_state(self):
        """定期发布舵机状态"""
        msg = JointState()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.name = ['yaw', 'pitch', 's2', 's3']

        # 角度单位转换：度 → 弧度
        msg.position = [
            angle * 3.14159265 / 180.0
            for angle in self.servo_angles
        ]

        # 模拟速度和电流（0 表示空闲）
        msg.velocity = [0.0] * 4
        msg.effort = [0.0] * 4

        self.servo_state_pub.publish(msg)


def main(args=None):
    rclpy.init(args=args)

    node = MockUARTBridge()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        print("\n[Mock UART Bridge] Shutting down...")
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
