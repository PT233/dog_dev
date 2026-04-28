# 项目状态快照

最后更新：`2026-04-28`

该文件不再维护旧的逐任务流水账，改为记录当前代码基线和需要继续推进的事项。

## 当前代码基线

- 文档已按当前重构后的代码树重写
- 视觉主链入口是 `robot_bringup/vision_stack.launch.py`
- 树莓派入口是 `robot_bringup/rpi_stack.launch.py`
- STM32 侧已经实现 UART 握手、状态回传、四路舵机轨迹规划

## 当前可确认的主链

1. Pi 摄像头推流：`scripts/start_camera_stream.sh`
2. WSL2 拉流与视觉处理：`gst_receiver -> stereo_splitter -> detection -> tracker -> behavior -> leg_motion`
3. Pi 串口桥：`uart_bridge_node`
4. STM32 执行：`Task_UART_RX -> Task_Traj_Planner -> Servo PWM`

## 当前未闭环项

- `/calibrate_center` 仍然只有接口定义，没有服务端
- 缺少覆盖整条主链的自动化端到端回归测试
- 硬件联调结果需要按当前重构后的文档重新做一次基线验证

## 当前建议优先级

1. 重新完成一次硬件端到端验收并记录结果
2. 给 `behavior_node` 增补 `/calibrate_center` 或删掉未使用接口
3. 增加 launch 级和 topic 级自动化 smoke test
