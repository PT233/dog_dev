# 任务 7.3：端到端联调 —— 静止物体追踪

**目标**：验证系统能够对静止杯子进行稳定追踪和锁定

**硬件就绪检查清单**：
- [ ] STM32 固件已烧录（flash.sh 输出 ✓ 固件已成功烧录）
- [ ] 树莓派 UART 已与 STM32 连接（看到 [STATUS] 串口输出）
- [ ] 相机能正常推流（rqt_image_view 能看到画面）
- [ ] 所有节点能启动（vision_stack.launch.py 完成）

---

## 🚀 快速启动（5 分钟）

### 1️⃣ 查看完整操作步骤

```bash
bash scripts/start_test_73.sh
```

输出包含：
- 4 个终端需要运行的命令
- 预期输出
- 故障排查提示

### 2️⃣ 按顺序启动系统（4 个终端）

**Terminal 1 - 树莓派：相机推流**
```bash
ssh ubuntu@192.168.137.100
export ROS_DOMAIN_ID=42
./scripts/start_camera_stream.sh
```

**Terminal 2 - 树莓派：ROS 节点**
```bash
ssh ubuntu@192.168.137.100
source ~/ros2_ws/install/setup.bash
export ROS_DOMAIN_ID=42
ros2 launch robot_bringup rpi_stack.launch.py
```

**Terminal 3 - WSL2：视觉管道**
```bash
cd /home/peter/dog/dog_dev
source install/setup.bash
export ROS_DOMAIN_ID=42
ros2 launch robot_bringup vision_stack.launch.py
```

等待所有节点启动（~10 秒），观察日志出现：
```
[gst_receiver_node] publishing to /stereo/image_raw
[detection_node] model loaded
[tracker_node] initialized
[behavior_node] started
[visual_servo_node] started
```

### 3️⃣ 准备测试环境

- 在摄像机视野内放置一个**杯子**
- 确保光线充足
- 杯子应该离摄像机 30-50cm

### 4️⃣ 切换目标类别为杯子

在 **Terminal 4** 中运行：

```bash
export ROS_DOMAIN_ID=42
ros2 service call /set_target_class robot_interfaces/srv/SetTargetClass "{class_name: 'cup'}"
```

**预期输出**：
```
requester: making request #1: ...SetTargetClass_Request(class_name='cup')
response: ...SetTargetClass_Response(success=true)
```

### 5️⃣ 验证追踪效果

在 **Terminal 4** 中运行自动验证脚本：

```bash
cd /home/peter/dog/dog_dev
bash scripts/verify_tracking.sh
```

脚本会输出：
```
[1/5] 检查 ROS 节点...      ✓ 6个节点就绪
[2/5] 检查话题频率...      ✓ 所有话题 >20Hz
[3/5] 检查 /tracked_objects... ✓ 收到追踪数据
[4/5] 采样像素误差...      ✓ 采集 10 秒数据
[5/5] 验收标准检查...
  最终像素误差 (x): 3 pixels
  ✓ 误差 < 5px（死区内）
  ✓ 任务 7.3 验收通过
```

---

## ✅ 验收标准

| 标准 | 检查方法 | 预期结果 |
|------|---------|--------|
| 能检测到杯子 | `ros2 topic echo /detections --limit 1` | 有输出（bbox + class） |
| 能追踪杯子 | `ros2 topic echo /tracked_objects --limit 1` | track_id 一致 |
| 误差收敛 | `bash scripts/verify_tracking.sh` | 最终误差 < 5px |
| 无持续振荡 | `rqt_plot /pixel_error/x /pixel_error/y` | 曲线平滑，无周期性抖动 |

---

## 🔧 常见问题

### 问题 1：无法检测到杯子

**症状**：`/detections` 或 `/tracked_objects` 无输出

**检查清单**：
1. 杯子是否在摄像机视野内？
   ```bash
   rqt_image_view  # 查看 /camera/image_mono
   ```

2. 杯子颜色是否足够鲜明？
   - YOLOv8 在自然光线下效果最好
   - 避免过亮或过暗的背景

3. 目标类别是否正确？
   ```bash
   # 验证目标类别
   ros2 topic echo /pixel_error --limit 1
   ```

4. detection_node 是否在运行？
   ```bash
   ros2 node list | grep detection
   ```

**解决方案**：
- 调整光线或杯子位置
- 用 `ros2 topic hz /detections` 检查是否有输出
- 查看 detection_node 日志（Terminal 3）

### 问题 2：舵机不响应

**症状**：系统正常运行，但舵机不转

**检查清单**：
1. `/servo_cmd` 话题是否有输出？
   ```bash
   ros2 topic echo /servo_cmd --limit 5
   ```

2. `/servo_state` 是否有反馈？
   ```bash
   ros2 topic echo /servo_state --limit 5
   ```

3. UART 连接是否正常？
   ```bash
   ssh ubuntu@192.168.137.100
   ls -l /dev/ttyAMA0  # 应该存在
   ```

4. STM32 是否有电源和 GND？

**解决方案**：
- 检查树莓派到 STM32 的 UART 接线（PA9/PA10 ↔ GPIO14/15）
- 重启 uart_bridge_node（Terminal 2 Ctrl+C，重新启动）
- 检查 STM32 串口输出（minicom -D /dev/ttyAMA0 -b 921600）

### 问题 3：误差不收敛

**症状**：`verify_tracking.sh` 输出误差 > 5px

**可能原因**：
1. **PID 参数不匹配**
   - Kp 太小 → 响应慢
   - Kp 太大 → 振荡
   - 见下方"参数调整"

2. **机械死区**
   - SG90 舵机通常有 ±3~5px 固有死区
   - 无法进一步改进

3. **杯子移动**
   - 杯子需要完全静止
   - 检查是否有风吹或物体移动

**调试步骤**：
```bash
# 1. 实时监控像素误差
ros2 topic echo /pixel_error

# 2. 查看舵机命令
ros2 topic echo /servo_cmd

# 3. 绘制误差曲线
rqt_plot /pixel_error/x /pixel_error/y &

# 4. 手动调整 PID（无需重启）
ros2 param set /visual_servo_node yaw.kp 0.06
```

---

## 📊 监控和调试

### 实时监控话题

```bash
# 像素误差（30Hz）
ros2 topic hz /pixel_error

# 追踪对象（30Hz）
ros2 topic hz /tracked_objects

# 舵机命令（30Hz）
ros2 topic hz /servo_cmd
```

### 可视化工具

```bash
# 查看摄像机图像
rqt_image_view

# 实时绘制误差曲线（推荐用于调参）
rqt_plot /pixel_error/x /pixel_error/y &

# 参数动态调整面板
rqt_reconfigure

# 完整 ROS 图形界面
rqt
```

### 节点和话题检查

```bash
# 列出所有运行的节点
ros2 node list

# 查看节点的话题订阅和发布
ros2 node info /behavior_node

# 列出所有话题
ros2 topic list

# 查看话题消息详情
ros2 topic info /pixel_error
```

---

## 🎛️ PID 参数快速调整

当前默认参数（config/visual_servo.yaml）：
```yaml
yaw:
  kp: 0.05
  ki: 0.001
  kd: 0.02
pitch:
  kp: 0.04
  ki: 0.001
  kd: 0.02
```

### 调整策略

| 观察到的现象 | 调整方案 | 预期改善 |
|-------------|--------|---------|
| 反应迟钝，追踪滞后 | Kp ↑（0.05 → 0.07） | 响应更快 |
| 振荡、过冲过大 | Kp ↓ 或 Kd ↑ | 震荡消除 |
| 稳态有偏差 | Ki ↑（0.001 → 0.002） | 消除偏差 |
| 动作抖动 | Kd ↑ 或 Ki ↓ | 动作平滑 |

### 动态调整示例

无需重启，直接修改参数：

```bash
# 增加 Kp
ros2 param set /visual_servo_node yaw.kp 0.07

# 增加 Kd（阻尼）
ros2 param set /visual_servo_node yaw.kd 0.03

# 观察曲线变化
rqt_plot /pixel_error/x /pixel_error/y &
```

---

## 📝 记录结果

测试成功后，更新 progress.md：

```bash
# 标记任务完成
# 记录当前 PID 参数
# 记录测试日期和最终误差

git add progress.md
git commit -m "任务 7.3：静止物体追踪端到端测试通过"
```

---

## 🔄 下一步（任务 7.4）

静止物体追踪成功后，进行任务 7.4：

```bash
bash scripts/start_test_74.sh  # （待创建）
```

任务 7.4 将：
1. 手持杯子缓慢移动（10cm/s）
2. 观察舵机跟踪响应
3. 用 rqt_plot 记录误差曲线
4. 根据曲线微调 PID 参数
5. 最终确保追踪精度 ±30px

---

**Last Updated**: 2026-04-24

