#!/usr/bin/env python3
"""
树莓派 UART 连通性测试脚本 - 任务2.1
功能：与 STM32 进行 921600bps UART 通信测试
- 发送舵机控制帧（servo0 转到90度，耗时1000ms）
- 持续接收和显示状态反馈帧
"""

import serial
import time
import struct
import sys


def crc16_ccitt(data):
    """CRC16-CCITT 计算（和 shared/uart_protocol.c 一致）"""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc <<= 1
            if crc & 0x10000:
                crc ^= 0x1021
            crc &= 0xFFFF
    return crc


def build_servo_cmd_frame(servo_id, angle_x10, duration_ms):
    """
    构造舵机控制帧（CMD_ID=0x01）
    格式：HEADER(2) + CMD_ID(1) + LEN(1) + PAYLOAD + CRC16(2) + TAIL(1)
    """
    # ServoCmdItem: servo_id(1) + angle_x10(2) + duration_ms(2)
    payload = struct.pack('<BHH', servo_id, angle_x10, duration_ms)

    cmd_id = 0x01  # servo control
    frame_len = len(payload)

    # CRC 覆盖：CMD_ID + LEN + PAYLOAD
    data_for_crc = bytes([cmd_id, frame_len]) + payload
    crc = crc16_ccitt(data_for_crc)

    # 完整帧
    frame = (
        bytes([0xAA, 0x55]) +           # HEADER
        bytes([cmd_id]) +                # CMD_ID
        bytes([frame_len]) +             # LEN
        payload +                        # PAYLOAD
        struct.pack('<H', crc) +         # CRC16 (little-endian)
        bytes([0x0D])                    # TAIL
    )

    return frame


def parse_status_frame(frame_bytes):
    """
    解析状态反馈帧（CMD_ID=0x81）
    返回 [(servo_id, current_angle_x10, status), ...]
    """
    if len(frame_bytes) < 6:
        return None

    # 最小帧长：HEADER(2) + CMD_ID(1) + LEN(1) + 1个Item(4) + CRC(2) + TAIL(1) = 12
    if frame_bytes[0:2] != bytes([0xAA, 0x55]):
        return None
    if frame_bytes[-1] != 0x0D:
        return None

    cmd_id = frame_bytes[2]
    if cmd_id != 0x81:
        return None

    frame_len = frame_bytes[3]
    if frame_len % 4 != 0:  # 每个 ServoStateItem = 4 字节
        return None

    # 提取 payload
    payload = frame_bytes[4:4+frame_len]

    # 验证 CRC
    data_for_crc = frame_bytes[2:2+2+frame_len]  # CMD_ID + LEN + PAYLOAD
    crc_bytes = frame_bytes[4+frame_len:4+frame_len+2]
    expected_crc = struct.unpack('<H', crc_bytes)[0]
    calculated_crc = crc16_ccitt(data_for_crc)

    if calculated_crc != expected_crc:
        print(f"  ⚠️  CRC 错误：期望 {expected_crc:04X}，得到 {calculated_crc:04X}")
        return None

    # 解析所有舵机状态项
    items = []
    for i in range(frame_len // 4):
        offset = i * 4
        servo_id, angle_x10, status = struct.unpack_from('<BHB', payload, offset)
        items.append((servo_id, angle_x10, status))

    return items


def main():
    """主测试程序"""
    port = '/dev/ttyAMA0'
    baudrate = 921600
    timeout = 1.0

    print("=" * 60)
    print("树莓派 UART 连通性测试 (任务2.1)")
    print("=" * 60)
    print(f"端口: {port}")
    print(f"波特率: {baudrate} bps")
    print(f"超时: {timeout} s")
    print()

    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        print(f"✅ 成功打开 {port}")
    except FileNotFoundError:
        print(f"❌ 错误：找不到 {port}")
        print("   请检查：")
        print("   1. 树莓派是否已经 reboot（修改 /boot/firmware/config.txt 后）")
        print("   2. STM32 是否已通过 GPIO UART 连接到树莓派")
        sys.exit(1)
    except Exception as e:
        print(f"❌ 打开串口失败：{e}")
        sys.exit(1)

    # 清空缓冲区
    time.sleep(0.5)
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    print("\n--- 测试 1：发送舵机控制帧 ---")
    print("命令：servo 0 转到 90°，耗时 1000ms")
    print(f"  servo_id=0, angle=900 (90.0°), duration=1000ms")

    # 构造帧：servo0 转到 90° 耗时 1000ms
    frame = build_servo_cmd_frame(servo_id=0, angle_x10=900, duration_ms=1000)
    frame_hex = ' '.join(f'{b:02X}' for b in frame)
    print(f"  帧内容：{frame_hex}")
    print(f"  帧长：{len(frame)} 字节")

    try:
        ser.write(frame)
        print(f"  ✅ 已发送 {len(frame)} 字节")
    except Exception as e:
        print(f"  ❌ 发送失败：{e}")
        ser.close()
        sys.exit(1)

    print("\n--- 测试 2：接收状态反馈帧（20 秒内）---")
    print("期望：STM32 每 50ms 发一次状态帧（CMD_ID=0x81）")
    print()

    start_time = time.time()
    received_count = 0
    error_count = 0
    frame_buffer = bytearray()

    while time.time() - start_time < 20:
        try:
            # 逐字节读取
            data = ser.read(1)
            if not data:
                continue

            frame_buffer.extend(data)

            # 简单帧同步：寻找帧头 0xAA 0x55
            while len(frame_buffer) >= 2:
                if frame_buffer[0:2] == bytes([0xAA, 0x55]):
                    # 寻找帧尾 0x0D
                    frame_end = -1
                    for i in range(2, len(frame_buffer)):
                        if frame_buffer[i] == 0x0D:
                            frame_end = i
                            break

                    if frame_end != -1:
                        # 找到完整帧
                        frame_data = bytes(frame_buffer[:frame_end+1])
                        frame_buffer = frame_buffer[frame_end+1:]

                        # 尝试解析
                        result = parse_status_frame(frame_data)
                        if result:
                            received_count += 1
                            timestamp = time.strftime('%H:%M:%S', time.localtime())
                            print(f"[{timestamp}] ✅ 帧 #{received_count}：", end="")
                            for servo_id, angle_x10, status in result:
                                angle_deg = angle_x10 / 10.0
                                status_str = {0: "idle", 1: "moving", 2: "error"}.get(status, "unknown")
                                print(f" servo{servo_id}={angle_deg:.1f}° ({status_str})", end="")
                            print()
                        else:
                            error_count += 1
                            frame_hex = ' '.join(f'{b:02X}' for b in frame_data[:min(20, len(frame_data))])
                            print(f"⚠️  坏帧 #{error_count}（长 {len(frame_data)}B）：{frame_hex}...")
                    else:
                        break  # 等待更多数据
                else:
                    # 移除一个不匹配的字节
                    frame_buffer.pop(0)

        except Exception as e:
            print(f"❌ 读取错误：{e}")
            break

    ser.close()

    print("\n" + "=" * 60)
    print("测试结果汇总")
    print("=" * 60)
    print(f"✅ 成功接收帧数：{received_count}")
    print(f"⚠️  坏帧数：{error_count}")

    if received_count >= 3:
        print("\n✅ 验收通过！STM32 与树莓派 UART 连通正常。")
        print("   - 舵机应该在这 20 秒内转到 90° 并维持")
        print("   - 物理上应该听到舵机齿轮转动声")
        return 0
    else:
        print("\n❌ 验收失败！")
        print("   请检查：")
        print("   1. STM32 是否已烧录任务 1.5 固件")
        print("   2. GPIO UART 连接（RXD/TXD 交叉）")
        print("   3. 电源是否正常")
        return 1


if __name__ == '__main__':
    sys.exit(main())
