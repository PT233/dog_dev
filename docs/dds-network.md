# DDS 网络配置

当前工程统一通过 `scripts/ros2_network_env.sh` 配置 CycloneDDS。

## 1. 设计目标

- 固定到单一网卡
- 关闭多播发现
- 显式写入单播 peers
- 统一 `ROS_DOMAIN_ID=42`

## 2. WSL2 用法

```bash
cd /home/peter/dog/dog_dev
export ROBOT_DDS_ROLE=wsl
source scripts/ros2_network_env.sh
```

默认会：

- 自动选取通往树莓派的出接口
- 把树莓派 IP 写入 CycloneDDS peers

## 3. Raspberry Pi 用法

```bash
cd /home/ubuntu/desktop_tracking_robot
export ROBOT_DDS_ROLE=rpi
export ROBOT_WSL_IP=<WSL_IP>
source scripts/ros2_network_env.sh
```

## 4. 常用变量

| 变量 | 含义 |
| --- | --- |
| `ROBOT_DDS_ROLE` | `wsl` 或 `rpi` |
| `ROBOT_PI_IP` | 树莓派 IP，WSL2 侧使用 |
| `ROBOT_WSL_IP` | WSL2 IP，Pi 侧使用 |
| `ROBOT_DDS_INTERFACE` | 强制绑定的网卡名 |
| `ROBOT_DDS_PEERS` | 手动覆盖 peers 列表 |

## 5. 成功后的环境变量

- `ROS_DOMAIN_ID=42`
- `ROS_LOCALHOST_ONLY=0`
- `RMW_IMPLEMENTATION=rmw_cyclonedds_cpp`
- `CYCLONEDDS_URI=file:///tmp/desktop_tracking_robot_cyclonedds_<role>.xml`

## 6. 诊断

```bash
echo "$ROS_DOMAIN_ID"
echo "$RMW_IMPLEMENTATION"
echo "$CYCLONEDDS_URI"
ros2 node list
ros2 topic list
```

如果两端可以互相 `ping` 但看不到 ROS 节点，优先检查：

1. 两端是否都 source 了这个脚本
2. `ROBOT_DDS_INTERFACE` 是否选错
3. `ROBOT_DDS_PEERS` 是否写成了旧 IP
