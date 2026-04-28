# Raspberry Pi 部署

## 1. 推荐方式

直接使用：

```bash
cd /home/peter/dog/dog_dev
scripts/rpi_one_click_deploy.sh --pi ubuntu@192.168.137.100
```

## 2. 这个脚本会做什么

- 同步 `robot_interfaces`、`uart_bridge`、`robot_bringup`
- 同步 Pi 端脚本到 `/home/ubuntu/desktop_tracking_robot/scripts`
- 写入 ROS 2 网络环境到 `.bashrc`
- 在 Pi 上构建或解压安装产物
- 可选启用开机服务

## 3. 用交叉编译产物部署

```bash
export RPI_SYSROOT=$HOME/rpi-sysroot
scripts/rpi_cross_build.sh
scripts/rpi_one_click_deploy.sh \
  --prebuilt-tar deploy/ros2_ws_install_rpi_aarch64.tgz
```

## 4. 只在 Pi 本地构建

```bash
ssh ubuntu@192.168.137.100
source /opt/ros/jazzy/setup.bash
cd ~/ros2_ws
colcon build --merge-install \
  --packages-select robot_interfaces uart_bridge robot_bringup
```

## 5. 部署后目录

| 路径 | 内容 |
| --- | --- |
| `~/ros2_ws/src/robot_interfaces` | 消息与服务 |
| `~/ros2_ws/src/uart_bridge` | UART 桥 |
| `~/ros2_ws/src/robot_bringup` | launch 文件 |
| `~/desktop_tracking_robot/scripts` | 启动与网络脚本 |

## 6. 部署后最小验证

```bash
ssh ubuntu@192.168.137.100
source /opt/ros/jazzy/setup.bash
source ~/ros2_ws/install/setup.bash
ros2 launch robot_bringup rpi_stack.launch.py
```
