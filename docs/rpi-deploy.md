# Raspberry Pi 一键部署

树莓派侧按当前架构只部署三个 ROS 2 包：

- `robot_interfaces`
- `uart_bridge`
- `robot_bringup`

视觉链路、YOLO/ONNX Runtime、跟踪和视觉伺服继续留在 WSL2/GPU 端。

## 默认方式：接线后远端源码构建

```bash
cd /home/peter/dog/dog_dev
./scripts/rpi_one_click_deploy.sh
```

脚本会同步 Pi 侧最小包集和启动脚本，排除 `build/`、`install/`、`log/`，然后在树莓派上执行：

```bash
colcon build --packages-select robot_interfaces uart_bridge robot_bringup
```

部署脚本默认还会安装并启用两个开机自启动服务：

- `desktop-tracking-camera.service`：启动 `/dev/video0` 到 WSL2 的 UDP 推流，并在推流后执行 `~/camera.sh`
- `desktop-tracking-rpi-stack.service`：启动 `robot_bringup rpi_stack.launch.py`

这一步需要树莓派上的 `ubuntu` 用户能执行无密码 `sudo`。如果当前只想同步和构建，不改 systemd：

```bash
./scripts/rpi_one_click_deploy.sh --no-boot-services
```

如果要关闭已经安装的开机服务：

```bash
./scripts/rpi_one_click_deploy.sh --disable-boot-services
```

默认只做不占用硬件的 smoke test。需要同时短暂验证 `/dev/ttyAMA0` 和 `/dev/video0` 时：

```bash
./scripts/rpi_one_click_deploy.sh --hardware-smoke
```

需要部署后立即启动服务：

```bash
./scripts/rpi_one_click_deploy.sh --start
```

## ARM64 预编译包方式

如果 WSL2 已准备好 ARM64 sysroot：

```bash
export RPI_SYSROOT=$HOME/rpi-sysroot
./scripts/rpi_cross_build.sh
./scripts/rpi_one_click_deploy.sh --prebuilt-tar deploy/ros2_ws_install_rpi_aarch64.tgz
```

没有 sysroot 时，交叉编译脚本会停在前置检查，并打印需要从树莓派同步的目录。ROS 2 的 ARM64 交叉编译需要目标机的 `/opt/ros/jazzy` 和 ARM64 系统库；只装 `aarch64-linux-gnu-g++` 不够。

## 运行入口

部署后树莓派上可直接运行：

```bash
~/desktop_tracking_robot/scripts/start_camera_stream.sh 192.168.137.1 5600
~/desktop_tracking_robot/scripts/rpi_start_ros.sh
```

`start_camera_stream.sh` 会在 GStreamer 推流启动后自动尝试执行 `~/camera.sh`，用于打开 3D webcam 的双目模式。

查看开机服务状态：

```bash
sudo systemctl status desktop-tracking-camera.service
sudo systemctl status desktop-tracking-rpi-stack.service
```
