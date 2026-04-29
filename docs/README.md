# 文档索引

本文档目录已经按当前代码结构重整，按主题阅读即可。

## 架构与拓扑

- [system-topology.md](system-topology.md): 三个运行域的总拓扑
- [ros2-topology.md](ros2-topology.md): ROS 2 包、节点、topic 和 service 关系
- [module-interactions.md](module-interactions.md): 代码模块之间的依赖边界
- [vision-to-motion-dataflow.md](vision-to-motion-dataflow.md): 从视频输入到舵机输出的数据流
- [stm32-firmware-code-doc.md](stm32-firmware-code-doc.md): STM32F103 固件的时钟、外设、任务、协议与调试说明
- [gst_receiver-node-code.md](gst_receiver-node-code.md): `gst_receiver_node` 的逐段代码说明、接口表和排错建议
- [stereo_splitter-node-code.md](stereo_splitter-node-code.md): `stereo_splitter_node` 的裁切逻辑、接口表和调试说明
- [uart_bridge-node-code.md](uart_bridge-node-code.md): `uart_bridge_node` 的串口桥接、握手状态机和线程模型说明

## 部署与启动

- [rpi-deploy.md](rpi-deploy.md): 树莓派部署路径
- [rpi-quickstart.md](rpi-quickstart.md): 树莓派侧快速启动
- [dds-network.md](dds-network.md): WSL2 与树莓派的 CycloneDDS 网络配置
- [verification.md](verification.md): 验证清单与诊断命令

## UART 与时序

- [uart-files.md](uart-files.md): UART 相关代码文件地图
- [time-sync.md](time-sync.md): `SERVO_STATE_V2` 时间戳映射逻辑
- [uart-idle-reset.md](uart-idle-reset.md): STM32 DMA + IDLE 收包与解析器重置行为
- [uart-packet-loss.md](uart-packet-loss.md): 丢包检测机制和排查方法
- [sg90-handshake-todo.md](sg90-handshake-todo.md): 握手与舵机上电链路的剩余待办
