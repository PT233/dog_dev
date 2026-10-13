#!/bin/bash
# Hardware-free regression test for mpu6050_lab.ko using the i2c-stub module.
# Run as root on the machine under test (not on the dev box):
#   sudo ./tests/mpu_stub_test.sh
# Only touches the bus whose adapter name is "SMBus stub driver" and unloads
# everything it loaded on exit.
set -u
export LC_ALL=C
cd "$(dirname "$0")/.."
ADDR=0x68
PASS=0; FAIL=0
ok()   { PASS=$((PASS+1)); printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf 'FAIL  %s\n' "$1"; }
check(){ if eval "$2"; then ok "$1"; else bad "$1  [$2]"; fi; }

cleanup() {
	[ -n "${BUS:-}" ] && echo $ADDR > /sys/bus/i2c/devices/i2c-$BUS/delete_device 2>/dev/null
	rmmod mpu6050_lab 2>/dev/null
	rmmod i2c_stub 2>/dev/null
}
trap cleanup EXIT

# MAKE_ARGS lets the host override the toolchain, e.g. "CC=gcc CONFIG_INIT_STACK_ALL_ZERO="
make -s ${MAKE_ARGS:-} 2>&1 | grep -E 'warning:|error' | grep -v 'compiler differs' && { echo "build has warnings"; exit 1; }
[ -x tools/mpu_dump ] || gcc -O2 -Wall -o tools/mpu_dump tools/mpu_dump.c -lm || exit 1
[ -x tools/stub_set ] || gcc -O2 -Wall -o tools/stub_set tools/stub_set.c || exit 1

modprobe i2c-dev
modprobe i2c-stub chip_addr=$ADDR || { echo "no i2c-stub"; exit 1; }
BUS=""
for d in /sys/bus/i2c/devices/i2c-*; do
	[ "$(cat $d/name)" = "SMBus stub driver" ] && BUS=${d##*i2c-}
done
[ -n "$BUS" ] || { echo "stub bus not found"; exit 1; }
echo "stub bus: i2c-$BUS"

# WHO_AM_I, then accel(256,-256,16384) temp(0) gyro(131,0,-131) at 0x3b..0x48
./tools/stub_set $BUS $ADDR 0x75 0x68 \
	0x3b 0x01 0x3c 0x00 0x3d 0xff 0x3e 0x00 0x3f 0x40 0x40 0x00 \
	0x41 0x00 0x42 0x00 0x43 0x00 0x44 0x83 0x45 0x00 0x46 0x00 \
	0x47 0xff 0x48 0x7d

DM0=$(dmesg | wc -l)
insmod ./mpu6050_lab.ko || { echo "insmod failed"; exit 1; }
echo mpu6050-lab $ADDR > /sys/bus/i2c/devices/i2c-$BUS/new_device
sleep 0.5
N=$(printf 'mpu6050_%d-%04x' $BUS $ADDR)
D=/dev/$N
S=/sys/bus/i2c/devices/$BUS-$(printf '%04x' $ADDR)
check "probe: device node exists" "[ -c $D ]"

out=$(./tools/mpu_dump $D 1 -v)
check "decode: accel/gyro raw values" \
	"echo '$out' | grep -q 'a=   256   -256  16384  g=   131      0   -131'"
check "decode: temperature 36.53 C" "echo '$out' | grep -q 'T=36.53C'"

stat_mean() { echo "$1" | sed -n 's/.*mean=\([0-9.]*\).*/\1/p'; }
within()    { awk -v v="$1" -v t="$2" -v p="$3" 'BEGIN{d=v-t; if(d<0)d=-d; exit !(d<=t*p)}'; }

r=$(./tools/mpu_dump $D 500)
check "100 Hz mean interval within 2% of 10000 us ($(stat_mean "$r"))" \
	"within $(stat_mean "$r") 10000 0.02"
echo 1000 > $S/sample_rate_hz
check "rate readback is 1000" "[ \"\$(cat $S/sample_rate_hz)\" = 1000 ]"
r=$(./tools/mpu_dump $D 3000)
check "1 kHz mean interval within 3% of 1000 us ($(stat_mean "$r"))" \
	"within $(stat_mean "$r") 1000 0.03"
check "1 kHz: no zero-length interval (duplicate timestamp)" \
	"! echo '$r' | grep -q 'min=0.0'"

echo 200 > $S/sample_rate_hz
r=$(./tools/mpu_dump $D 200 -n)
check "O_NONBLOCK + poll delivers 200 samples" "echo '$r' | grep -q 'samples=200'"

echo 5000 > $S/sample_rate_hz 2>/dev/null
check "rate 5000 rejected" "[ \$? -ne 0 ]"
echo abc > $S/sample_rate_hz 2>/dev/null
check "rate 'abc' rejected" "[ \$? -ne 0 ]"

echo 1000 > $S/sample_rate_hz
ov0=$(awk '/fifo_overflow/{print $2}' $S/stats)
r=$(./tools/mpu_dump $D 300 -d 600)
ov1=$(awk '/fifo_overflow/{print $2}' $S/stats)
check "FIFO overflow counted (+$((ov1-ov0)))" "[ $((ov1-ov0)) -gt 100 ]"
check "dropped_before reported to reader" "echo '$r' | grep -qE 'dropped=[1-9][0-9]*'"

( ./tools/mpu_dump $D 100000 >/dev/null 2>&1 & echo $! > /tmp/mpu_pid; sleep 0.2
  ./tools/mpu_dump $D 1 >/dev/null 2>&1; echo $? > /tmp/mpu_rc2; wait )
check "second open returns EBUSY" "[ \"\$(cat /tmp/mpu_rc2)\" = 1 ]"

sleep $((2 + 1))
check "runtime PM: suspended after autosuspend delay" \
	"[ \"\$(cat $S/power/runtime_status)\" = suspended ]"
check "runtime PM: usage count balanced" \
	"[ \"\$(cat $S/power/runtime_usage)\" = 0 ]"

# --- lifetime: remove() while a reader is blocked in read() -----------------
./tools/mpu_dump $D 100000000 >/tmp/mpu_f.txt 2>&1 &
RP=$!
sleep 0.5
echo $ADDR > /sys/bus/i2c/devices/i2c-$BUS/delete_device
for i in $(seq 20); do kill -0 $RP 2>/dev/null || break; sleep 0.1; done
if kill -0 $RP 2>/dev/null; then bad "blocked reader exits after device removal"; kill -9 $RP
else ok "blocked reader exits after device removal"; fi
check "device node removed" "[ ! -e $D ]"
check "module unloads once the file is closed" "rmmod mpu6050_lab"

# --- rebind loop -------------------------------------------------------------
insmod ./mpu6050_lab.ko
fails=0
for i in $(seq 50); do
	echo mpu6050-lab $ADDR > /sys/bus/i2c/devices/i2c-$BUS/new_device 2>/dev/null
	./tools/mpu_dump $D 20 >/dev/null 2>&1 || fails=$((fails+1))
	echo $ADDR > /sys/bus/i2c/devices/i2c-$BUS/delete_device
done
check "50x bind/read/unbind without failures" "[ $fails -eq 0 ]"

# --- open file survives unbind: no use-after-free ---------------------------
echo mpu6050-lab $ADDR > /sys/bus/i2c/devices/i2c-$BUS/new_device
sleep 0.3
exec 7<$D
echo $ADDR > /sys/bus/i2c/devices/i2c-$BUS/delete_device
LC_ALL=C dd bs=32 count=1 <&7 >/dev/null 2>/tmp/mpu_dd.txt
check "read on orphaned fd fails cleanly (ENODEV)" "grep -q 'No such device' /tmp/mpu_dd.txt"
exec 7<&-

bad_lines=$(dmesg | tail -n +$((DM0+1)) | grep -ciE 'BUG:|WARNING:|Oops|lockdep|Call Trace|usage count|kmemleak|use-after-free')
check "no kernel BUG/WARNING/Oops in dmesg" "[ $bad_lines -eq 0 ]"

echo "-----"
echo "passed=$PASS failed=$FAIL"
[ $FAIL -eq 0 ]
