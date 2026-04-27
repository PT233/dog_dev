# WSL2 与树莓派 ROS 2 CycloneDDS 单播配置

## 问题

WSL2 不是直接运行在物理网卡上的 Linux。默认网络经过 Hyper-V 虚拟交换机和 NAT，这会影响 ROS 2 DDS：

- WSL2 的 IP 可能在 Windows 重启后变化，树莓派如果写死 peer IP 会失效。
- WSL2 默认多播支持不可靠，而 DDS 自动发现通常依赖多播。
- DDS 可能同时看到 Windows 网卡、WSL2 虚拟网卡等多个接口，导致 participant 重复或发现异常。

## 方案

统一使用 CycloneDDS，并通过 `CYCLONEDDS_URI` 指向运行时生成的 XML：

- 固定 `RMW_IMPLEMENTATION=rmw_cyclonedds_cpp`
- 指定单一网卡接口：
  - WSL2：默认使用到树莓派路由上的接口，通常是 `eth0`
  - 树莓派：默认使用 `eth0`
- 禁用多播发现：`AllowMulticast=false`
- 使用单播 peer：
  - WSL2 peer 指向树莓派：`192.168.137.100`
  - 树莓派 peer 指向当前 WSL2 IP 或 Windows/WSL 转发入口

统一入口脚本：

```bash
scripts/ros2_network_env.sh
```

该脚本会生成：

```bash
/tmp/desktop_tracking_robot_cyclonedds_wsl.xml
/tmp/desktop_tracking_robot_cyclonedds_rpi.xml
```

并导出：

```bash
ROS_DOMAIN_ID=42
ROS_LOCALHOST_ONLY=0
RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
CYCLONEDDS_URI=file:///tmp/desktop_tracking_robot_cyclonedds_<role>.xml
```

## WSL2 端

在启动视觉链路、验证脚本或 ROS CLI 前执行：

```bash
cd /home/peter/dog/dog_dev
source /opt/ros/jazzy/setup.bash
source ros2_ws/install/setup.bash

export ROBOT_DDS_ROLE=wsl
export ROBOT_PI_IP=192.168.137.100
source scripts/ros2_network_env.sh
```

如果自动选错接口，手动指定：

```bash
export ROBOT_DDS_INTERFACE=eth0
source scripts/ros2_network_env.sh
```

启动视觉链路：

```bash
ros2 launch robot_bringup vision_stack.launch.py
```

## 树莓派端

树莓派启动 `rpi_stack` 前执行：

```bash
cd ~/desktop_tracking_robot
source /opt/ros/jazzy/setup.bash
source ~/ros2_ws/install/setup.bash

export ROBOT_DDS_ROLE=rpi
export ROBOT_WSL_IP=<当前 WSL2 可达地址>
export ROBOT_DDS_INTERFACE=eth0
source scripts/ros2_network_env.sh
```

启动树莓派节点：

```bash
ros2 launch robot_bringup rpi_stack.launch.py
```

## WSL2 IP 变化处理

在 WSL2 中可用下面命令查看到树莓派路由使用的源地址和接口：

```bash
ip route get 192.168.137.100
```

项目的一键部署脚本会自动推导 `WSL_TARGET_IP` 并写入树莓派端环境：

```bash
scripts/rpi_one_click_deploy.sh
```

如果自动推导不符合实际网络，可以显式传入：

```bash
WSL_TARGET_IP=<当前 WSL2 可达地址> scripts/rpi_one_click_deploy.sh
```

Windows 重启或 WSL2 网络重建后，重新运行一键部署脚本或重新设置树莓派端 `ROBOT_WSL_IP`，避免 peer 指向旧地址。

## 验证

两端分别检查环境：

```bash
echo "$RMW_IMPLEMENTATION"
echo "$CYCLONEDDS_URI"
cat "${CYCLONEDDS_URI#file://}"
```

确认 XML 中：

- `NetworkInterface name` 是期望的网卡。
- `AllowMulticast` 为 `false`。
- `Peer Address` 指向对端地址。

ROS 验证：

```bash
ros2 node list
ros2 topic list
ros2 topic echo /servo_state --once
ros2 topic echo /servo_cmd --once
```

## 常见问题

### 找不到 rmw_cyclonedds_cpp

两端安装 CycloneDDS RMW：

```bash
sudo apt install ros-jazzy-rmw-cyclonedds-cpp
```

### WSL2 能 SSH 到树莓派，但 ROS 2 看不到节点

检查两端：

- `ROS_DOMAIN_ID` 是否一致。
- `RMW_IMPLEMENTATION` 是否都是 `rmw_cyclonedds_cpp`。
- `CYCLONEDDS_URI` 指向的 XML 是否存在。
- peer 地址是否是对端可达地址。
- Windows 防火墙是否放行 UDP DDS 流量。

### 指定接口后仍然不通

先分别测试单播可达性：

```bash
ping 192.168.137.100
nc -vz 192.168.137.100 22
```

然后检查路由：

```bash
ip route
ip addr
```

如果 WSL2 默认 NAT 地址无法从树莓派访问，需要使用 Windows 端端口转发/路由，或改用 WSL2 mirrored networking，再重新设置 `ROBOT_WSL_IP` 和 `ROBOT_DDS_INTERFACE`。
