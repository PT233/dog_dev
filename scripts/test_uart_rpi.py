#!/usr/bin/env python3
"""
树莓派 UART 连通性测试脚本 - 任务2.1
功能：与 STM32 进行 921600bps UART 通信测试
- 发送初始化握手帧，等待 STM32 ACTIVE
- 发送舵机控制帧（servo0 转到90度，耗时1000ms）
- 持续接收和显示状态反馈帧（0x82）
"""

import serial
import time
import struct
import sys

UART_PROTOCOL_VERSION = 0x03
UART_CMD_SERVO_CONTROL = 0x01
UART_CMD_INIT_HANDSHAKE = 0x10
UART_CMD_SERVO_STATE = 0x81
UART_CMD_SERVO_STATE_V2 = 0x82
UART_CMD_SYSTEM_STATE = 0x83
UART_SYSTEM_STATE_ACTIVE = 0x03

SYSTEM_STATE_NAMES = {
    0x01: "BOOT_CENTERING",
    0x02: "WAITING_CONNECTION",
    0x03: "ACTIVE",
    0x7F: "ERROR",
}


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


def build_frame(cmd_id, payload):
    """构造通用 UART 帧。"""
    frame_len = len(payload)
    data_for_crc = bytes([cmd_id, frame_len]) + payload
    crc = crc16_ccitt(data_for_crc)
    return (
        bytes([0xAA, 0x55]) +
        bytes([cmd_id, frame_len]) +
        payload +
        struct.pack('<H', crc) +
        bytes([0x0D])
    )


def build_init_handshake_frame():
    """构造 bridge 启动握手帧（CMD_ID=0x10）。"""
    payload = struct.pack('<BBH', UART_PROTOCOL_VERSION, UART_SYSTEM_STATE_ACTIVE, 0)
    return build_frame(UART_CMD_INIT_HANDSHAKE, payload)


def build_servo_cmd_frame(servo_id, angle_x10, duration_ms):
    """
    构造舵机控制帧（CMD_ID=0x01）
    格式：HEADER(2) + CMD_ID(1) + LEN(1) + PAYLOAD + CRC16(2) + TAIL(1)
    """
    # ServoCmdItem: servo_id(1) + angle_x10(2, signed) + duration_ms(2)
    payload = struct.pack('<BhH', servo_id, angle_x10, duration_ms)
    return build_frame(UART_CMD_SERVO_CONTROL, payload)


def parse_system_state_frame(frame_bytes):
    """解析 STM32 系统状态帧（CMD_ID=0x83）。"""
    if len(frame_bytes) < 7 or frame_bytes[0:2] != bytes([0xAA, 0x55]):
        return None
    if frame_bytes[-1] != 0x0D or frame_bytes[2] != UART_CMD_SYSTEM_STATE:
        return None

    payload_len = frame_bytes[3]
    if payload_len != 8:
        return None

    data_for_crc = frame_bytes[2:2+2+payload_len]
    expected_crc = struct.unpack('<H', frame_bytes[4+payload_len:4+payload_len+2])[0]
    calculated_crc = crc16_ccitt(data_for_crc)
    if calculated_crc != expected_crc:
        return None

    protocol_version, system_state, _reserved, uptime_ms = struct.unpack(
        '<BBHI', frame_bytes[4:4+payload_len]
    )
    return protocol_version, system_state, uptime_ms


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
    if cmd_id not in (UART_CMD_SERVO_STATE, UART_CMD_SERVO_STATE_V2):
        return None

    frame_len = frame_bytes[3]
    item_size = 4 if cmd_id == UART_CMD_SERVO_STATE else 8
    if frame_len % item_size != 0:
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
    for i in range(frame_len // item_size):
        offset = i * item_size
        if cmd_id == UART_CMD_SERVO_STATE:
            servo_id, angle_x10, status = struct.unpack_from('<BhB', payload, offset)
        else:
            servo_id, angle_x10, status, _timestamp_ms, _frame_seq = struct.unpack_from(
                '<BhBHH', payload, offset
            )
        items.append((servo_id, angle_x10, status))

    return items


def read_valid_frame(ser, deadline):
    """按 LEN 字段读取一帧，避免 payload/CRC 中的 0x0D 被误认为帧尾。"""
    frame_buffer = bytearray()

    while time.time() < deadline:
        data = ser.read(1)
        if not data:
            continue

        frame_buffer.extend(data)

        while len(frame_buffer) >= 2 and frame_buffer[0:2] != bytes([0xAA, 0x55]):
            frame_buffer.pop(0)

        if len(frame_buffer) < 4:
            continue

        expected_len = 7 + frame_buffer[3]
        if len(frame_buffer) < expected_len:
            continue

        frame = bytes(frame_buffer[:expected_len])
        del frame_buffer[:expected_len]

        if frame[-1] != 0x0D:
            continue

        payload_len = frame[3]
        expected_crc = struct.unpack('<H', frame[4+payload_len:4+payload_len+2])[0]
        calculated_crc = crc16_ccitt(frame[2:2+2+payload_len])
        if expected_crc != calculated_crc:
            continue

        return frame

    return None


def wait_for_handshake(ser, timeout=5.0):
    """发送握手并等待 STM32 回复 ACTIVE。"""
    handshake = build_init_handshake_frame()
    deadline = time.time() + timeout
    next_send = 0.0

    while time.time() < deadline:
        now = time.time()
        if now >= next_send:
            ser.write(handshake)
            next_send = now + 0.5

        frame = read_valid_frame(ser, min(deadline, time.time() + 0.2))
        if not frame:
            continue

        state = parse_system_state_frame(frame)
        if not state:
            continue

        protocol_version, system_state, uptime_ms = state
        state_name = SYSTEM_STATE_NAMES.get(system_state, f"UNKNOWN({system_state})")
        print(f"  STM32 state={state_name}, protocol={protocol_version}, uptime={uptime_ms}ms")

        if protocol_version == UART_PROTOCOL_VERSION and system_state == UART_SYSTEM_STATE_ACTIVE:
            return True

    return False


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

    print("\n--- 测试 1：初始化握手 ---")
    if not wait_for_handshake(ser, timeout=5.0):
        print("  ❌ 握手失败：STM32 未进入 ACTIVE，舵机控制命令不会被执行")
        ser.close()
        return 1
    print("  ✅ 握手完成，STM32 已进入 ACTIVE")

    print("\n--- 测试 2：发送舵机控制帧 ---")
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

    print("\n--- 测试 3：接收状态反馈帧（20 秒内）---")
    print("期望：STM32 每 50ms 发一次状态帧（CMD_ID=0x82）")
    print()

    start_time = time.time()
    received_count = 0
    error_count = 0

    while time.time() - start_time < 20:
        try:
            frame_data = read_valid_frame(ser, time.time() + 1.0)
            if not frame_data:
                continue

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
            elif frame_data[2] != UART_CMD_SYSTEM_STATE:
                error_count += 1
                frame_hex = ' '.join(f'{b:02X}' for b in frame_data[:min(20, len(frame_data))])
                print(f"⚠️  非状态帧 #{error_count}（长 {len(frame_data)}B）：{frame_hex}...")

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
