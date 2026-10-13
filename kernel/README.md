# kernel/ —— 机器狗的 Linux 内核驱动

源码同时兼容 Linux 5.15（树莓派上的 Ubuntu 22.04）与 6.8。

| 目录 | 是什么 | 在机器狗里的位置 |
|---|---|---|
| `dogcam/` | USB 相机驱动，替换内核自带的 `uvcvideo` | 驱动双目相机，出 `/dev/videoN` 给 `scripts/start_camera_stream.sh` |
| `mpu6050/` | MPU-6050 的 I²C 驱动，字符设备 `/dev/mpu6050_<bus>-<addr>` | `ros2_ws/src/imu_node` 读它并发布 `sensor_msgs/Imu` |
| `vcam/` | 相机的软件替身，V4L2 采集节点 | 没有相机时做回归测试 |

## dogcam：自己写的 UVC 驱动

只做一件事并做完整：非压缩 YUY2 走等时传输。

- **probe**：解析 VideoControl / VideoStreaming 的类描述符，收集 YUY2 的分辨率和帧间隔。
- **STREAMON**：`VS_PROBE` / `VS_COMMIT` 协商；在备用设置里选出能承载 `dwMaxPayloadTransferSize` 的最小端点；提交等时 URB。
- **URB 完成回调**：逐包解析负载头（FID、EOF、ERR），把负载拷进当前 vb2 缓冲；只有收满一整帧才交给用户态，残帧丢弃并计数。
- **STREAMOFF**：kill URB，回到备用设置 0。
- 和 `uvcvideo` 一样绑定 VideoControl 接口并认领 VideoStreaming 接口，所以同一台相机只能归其中一个。
- `UVCIOC_CTRL_QUERY` 原样透传到 VideoControl 接口，厂商用来切换模式的扩展单元命令不受影响。

没有静态 ID 表，需要显式绑定：

```
echo -n 1-6:1.0 | sudo tee /sys/bus/usb/drivers/uvcvideo/unbind   # 相机的 VideoControl 接口
sudo insmod kernel/dogcam/dogcam.ko
echo "05c8 0441" | sudo tee /sys/bus/usb/drivers/dogcam/new_id    # 相机的 VID PID
```

不支持：MJPEG 等压缩格式、bulk 端点、V4L2 控件、静态图像采集。

## vcam：为什么要有它

实机相机是 UVC 双目模组，由内核自带的 `uvcvideo` 驱动，输出 YUY2 640×480@30 的左右拼接帧，`stereo_splitter` 取 x∈[0,320) 作为左目。`vcam` 提供相同的采集格式和默认相同的左右拼接布局：

- 左半幅是彩条，右半幅是灰度渐变，裁错半幅一眼可见。
- 每行开头写 `{seq, ~seq}`，x=width/2 处写 `{~seq, seq}`，用户态可以校验左右目和帧号。
- 结构按真实驱动写：sensor 子设备（pad ops、控件）加采集桥（videobuf2、MMAP、EXPBUF），Media Controller 链路 `vcam-sensor:0 → vcam:0`。

用法：`sudo insmod kernel/vcam/vcam.ko` 后，把 `CAMERA_DEVICE` 指向它即可：

```
CAMERA_DEVICE=/dev/video2 H264_ENCODER='x264enc tune=zerolatency speed-preset=ultrafast key-int-max=30' \
  scripts/start_camera_stream.sh 127.0.0.1 5600
```

`tools/qbuf_race` 用它复现"DQBUF 后立刻 QBUF 再继续读映射内存"导致的帧覆盖，并对比深拷贝与延迟归还两种修法。

## mpu6050：采样路径

- DATA_RDY 上升沿 → 硬中断只记 `ktime_get_ns()` 入时间戳队列 → 中断线程取空队列、读一次 14 字节。线程来晚时被合并的事件计入 `dropped_before`。
- 设备树里没有 `interrupts` 时退化为 hrtimer + workqueue。
- `read()` 返回 32 字节定长记录（`mpu6050_lab_uapi.h`），支持 `poll`；sysfs 有 `sample_rate_hz` 和 `stats`。
- 没人打开时芯片休眠（runtime PM）；设备被移除时已打开的文件得到 `-ENODEV`。
- `mpu6050-lab.dts` 是树莓派 overlay（INT 接 GPIO17）。

`imu_node` 把原始值换算成 m/s² 和 rad/s，`header.stamp` 用驱动在中断里取的时间戳加一个时钟偏移，不用用户态线程被调度到的时刻。

## 验证

环境：Ubuntu 22.04 + Linux 6.8（x86_64）+ ROS 2 Humble，没有真实 IMU 和相机。

| 测试 | 内容 | 结果 |
|---|---|---|
| `kernel/dogcam/tests/dogcam_camera_test.sh` | 真实 UVC 摄像头，先测 `uvcvideo` 作参照，再换 `dogcam` 逐项对比 | 26/26 |
| `kernel/mpu6050/tests/mpu_stub_test.sh` | `i2c-stub` 虚拟芯片：解码、采样率、溢出、设备移除、重绑定 | 21/21 |
| `kernel/vcam/tests/vcam_test.sh` | 拓扑、v4l2-compliance、控件、格式、帧覆盖复现、unbind/bind | 25/25 |
| `tests/test_camera_pipeline_vcam.sh` | 推流脚本的采集格式、左右目布局、H.264/RTP 回环到 `gst_receiver` 的软解管线 | 10/10 |
| `tests/test_imu_node_stub.sh` | 驱动 → `imu_node` → `sensor_msgs/Imu`：单位、帧率、时间戳、设备移除 | 10/10 |

- `dogcam` 的实测对象是一台笔记本内置的 UVC 摄像头（05c8:0441，高速 USB，YUY2 640×480）：
  - 输出帧与 `uvcvideo` 逐字节相同（当时镜头是静态暗画面），60 帧耗时和帧率也一致。
  - 0 个残帧，0 个等时包错误；`v4l2src` 用推流脚本的格式协商成功，经 H.264/RTP 回环解出 101 帧。
  - 20 次起停、流传输中解绑再重绑均正常；`v4l2-compliance -s` 为 51/52。
- `vcam` 的 `v4l2-compliance -s` 为 52/53。两者唯一的失败项都来自 v4l-utils 1.22.1 比 6.8 内核老（`reserved[]` 的 ABI 差异），`vcam/tools/crbufs_probe` 可确认。
- 5.15 与 6.8 头文件下均无告警编译；`checkpatch --strict` 0 error，剩余为有意使用的 `LINUX_VERSION_CODE`。

## 还没验证

- **`dogcam` 没有在机器狗的双目相机上跑过。** 它按 UVC 描述符通用解析，不依赖具体型号，但双目相机的端点参数、是否带 EOF 标志、双目模式的开启方式（`~/camera.sh`，不在仓库里）都要上机确认。
- `dogcam` 只在 USB 2.0 高速设备上测过；SuperSpeed 的分支只有代码。
- 树莓派上的实机运行：真实 MPU-6050 和 INT 引脚（中断路径）、设备树 overlay 加载、armv7 编译。
- `imu_node` 还没有接进 `robot_bringup` 的启动文件，也没有下游节点使用 IMU 数据。
- RTP 回环在 PC 上用的是 `x264enc`，树莓派上的 `v4l2h264enc` 路径没有在这里跑。
- `gst_receiver_node` 和 `stereo_splitter_node` 本体没有参与回环（测试机缺 GStreamer 开发头），用的是与 `gst_receiver_node.cpp` 相同的管线字符串。
- 内核没有开 KASAN、lockdep、kmemleak。

## 在树莓派上

```
sudo apt install build-essential linux-headers-$(uname -r) device-tree-compiler
make -C kernel/mpu6050
dtc -@ -I dts -O dtb -o mpu6050-lab.dtbo kernel/mpu6050/mpu6050-lab.dts
sudo cp mpu6050-lab.dtbo /boot/firmware/overlays/
# /boot/firmware/config.txt 加 dtparam=i2c_arm=on 和 dtoverlay=mpu6050-lab 后重启
sudo insmod kernel/mpu6050/mpu6050_lab.ko
ros2 run imu_node imu_node
```
