#!/bin/bash
# imu_node + kernel/mpu6050 end to end, without an IMU: i2c-stub plays the chip.
#
#   (as the normal user)  cd ros2_ws; colcon build --packages-select imu_node
#   sudo MAKE_ARGS="..." ./tests/test_imu_node_stub.sh
#
# Checks that samples from the kernel driver come out of ROS 2 as
# sensor_msgs/Imu in SI units, at the configured rate, with driver timestamps.
set -u
export LC_ALL=C
REPO="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO"
ADDR=0x68
PASS=0; FAIL=0
ok()   { PASS=$((PASS+1)); printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf 'FAIL  %s\n' "$1"; }
check(){ if eval "$2"; then ok "$1"; else bad "$1  [$2]"; fi; }

NODE=ros2_ws/install/imu_node/lib/imu_node/imu_node
[ -x "$NODE" ] || { echo "build imu_node first: cd ros2_ws; colcon build --packages-select imu_node"; exit 1; }

cleanup() {
	[ -n "${NP:-}" ] && kill $NP 2>/dev/null
	[ -n "${BUS:-}" ] && echo $ADDR > /sys/bus/i2c/devices/i2c-$BUS/delete_device 2>/dev/null
	sleep 0.3
	rmmod mpu6050_lab 2>/dev/null
	rmmod i2c_stub 2>/dev/null
}
trap cleanup EXIT

M=kernel/mpu6050
make -s -C $M ${MAKE_ARGS:-} 2>&1 | grep -E 'warning:|error' | grep -v 'compiler differs' && { echo "build has warnings"; exit 1; }
[ -x $M/tools/stub_set ] || gcc -O2 -Wall -o $M/tools/stub_set $M/tools/stub_set.c || exit 1

modprobe i2c-dev
modprobe i2c-stub chip_addr=$ADDR || { echo "no i2c-stub"; exit 1; }
BUS=""
for d in /sys/bus/i2c/devices/i2c-*; do
	[ "$(cat $d/name)" = "SMBus stub driver" ] && BUS=${d##*i2c-}
done
[ -n "$BUS" ] || { echo "stub bus not found"; exit 1; }

# accel raw (256, -256, 16384)  gyro raw (131, 0, -131)
$M/tools/stub_set $BUS $ADDR 0x75 0x68 \
	0x3b 0x01 0x3c 0x00 0x3d 0xff 0x3e 0x00 0x3f 0x40 0x40 0x00 \
	0x41 0x00 0x42 0x00 0x43 0x00 0x44 0x83 0x45 0x00 0x46 0x00 \
	0x47 0xff 0x48 0x7d
DM0=$(dmesg | wc -l)
insmod $M/mpu6050_lab.ko || { echo "insmod failed"; exit 1; }
echo mpu6050-lab $ADDR > /sys/bus/i2c/devices/i2c-$BUS/new_device
sleep 0.5
D=/dev/$(printf 'mpu6050_%d-%04x' $BUS $ADDR)
S=/sys/bus/i2c/devices/$BUS-$(printf '%04x' $ADDR)
check "driver bound, $D present" "[ -c $D ]"
echo 100 > $S/sample_rate_hz

set +u
source /opt/ros/humble/setup.bash
source ros2_ws/install/setup.bash
set -u
export ROS_LOCALHOST_ONLY=1 ROS_DOMAIN_ID=87
T=/imu_node/output/imu_raw

$NODE --ros-args -p device:=$D >/tmp/imu_node.log 2>&1 &
NP=$!
sleep 2
check "imu_node is running" "kill -0 $NP"

timeout 15 ros2 topic echo --once $T >/tmp/imu_msg.yaml 2>/dev/null
res=$(python3 - <<'PY'
import math, time, yaml
m = next(d for d in yaml.safe_load_all(open('/tmp/imu_msg.yaml')) if d)
a, g = m['linear_acceleration'], m['angular_velocity']
G, D = 9.80665, math.pi / 180
near = lambda v, t: abs(v - t) < 1e-4
stamp = m['header']['stamp']['sec'] + m['header']['stamp']['nanosec'] * 1e-9
print('accel_ok=%d gyro_ok=%d frame=%s cov0=%.0f age_ok=%d' % (
    near(a['x'], 256 / 16384 * G) and near(a['y'], -256 / 16384 * G) and near(a['z'], G),
    near(g['x'], D) and near(g['y'], 0.0) and near(g['z'], -D),
    m['header']['frame_id'], m['orientation_covariance'][0],
    abs(time.time() - stamp) < 5.0))
PY
)
echo "      $res"
check "accel in m/s^2: (0.1532, -0.1532, 9.80665)" "grep -q 'accel_ok=1' <<<'$res'"
check "gyro in rad/s: (0.017453, 0, -0.017453)" "grep -q 'gyro_ok=1' <<<'$res'"
check "frame_id imu_link, orientation marked unavailable" "grep -q 'frame=imu_link cov0=-1' <<<'$res'"
check "header.stamp is driver time mapped to ROS time" "grep -q 'age_ok=1' <<<'$res'"

hz=$(timeout 8 ros2 topic hz $T --window 300 2>/dev/null | sed -n 's/^average rate: \([0-9.]*\).*/\1/p' | tail -1)
check "published at the configured 100 Hz (${hz:-none})" \
	"awk -v v=${hz:-0} 'BEGIN{exit !(v>95 && v<105)}'"

echo $ADDR > /sys/bus/i2c/devices/i2c-$BUS/delete_device
sleep 1
check "node reports the device going away" "grep -q 'went away' /tmp/imu_node.log"
kill $NP 2>/dev/null; wait $NP 2>/dev/null; NP=""
check "module unloads once the node has exited" "rmmod mpu6050_lab"
bad_lines=$(dmesg | tail -n +$((DM0+1)) | grep -ciE 'BUG:|WARNING:|Oops|Call Trace|use-after-free|usage count')
check "no kernel BUG/WARNING/Oops in dmesg" "[ $bad_lines -eq 0 ]"

echo "-----"
echo "passed=$PASS failed=$FAIL"
[ $FAIL -eq 0 ]
