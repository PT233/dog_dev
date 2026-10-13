#!/bin/bash
# Camera-less regression of the Pi-side video chain, using kernel/vcam as a
# stand-in for the USB stereo camera (YUY2 640x480@30, eyes side by side).
#
#   sudo MAKE_ARGS="..." GST_PLUGIN_PATH=... ./tests/test_camera_pipeline_vcam.sh
#
# What it proves without any camera attached:
#   1. the capture caps used by scripts/start_camera_stream.sh negotiate
#   2. frames arrive at 30 fps with the left/right layout stereo_splitter assumes
#   3. (if an H.264 encoder is available) start_camera_stream.sh -> RTP/UDP ->
#      the receive pipeline of gst_receiver_node decodes to 640x480 BGR with the
#      left eye still on the left
set -u
export LC_ALL=C
REPO="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO"
PASS=0; FAIL=0
ok()   { PASS=$((PASS+1)); printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf 'FAIL  %s\n' "$1"; }
note() { printf 'NOTE  %s\n' "$1"; }
check(){ if eval "$2"; then ok "$1"; else bad "$1  [$2]"; fi; }

cleanup() { pkill -f 'udpsrc port=5600' 2>/dev/null; pkill -f start_camera_stream.sh 2>/dev/null; sleep 0.3; rmmod vcam 2>/dev/null; }
trap cleanup EXIT

make -s -C kernel/vcam ${MAKE_ARGS:-} 2>&1 | grep -E 'warning:|error' | grep -v 'compiler differs' && { echo "build has warnings"; exit 1; }
[ -x kernel/vcam/tools/sbs_check ] || gcc -O2 -Wall -o kernel/vcam/tools/sbs_check kernel/vcam/tools/sbs_check.c || exit 1

DM0=$(dmesg | wc -l)
modprobe videobuf2-vmalloc && insmod kernel/vcam/vcam.ko || { echo "insmod failed"; exit 1; }
sleep 0.5
V=""
for d in /sys/class/video4linux/*; do
	[ "$(cat $d/name 2>/dev/null)" = vcam ] && V=/dev/$(basename $d)
done
check "vcam registered ($V)" "[ -c '$V' ]"

# --- 1+2: same caps as the Pi script, layout stereo_splitter relies on --------
CAPS=$(grep -o "video/x-raw,format=YUY2,width=[0-9]*,height=[0-9]*,framerate=[0-9]*/1" scripts/start_camera_stream.sh)
check "start_camera_stream.sh still asks for YUY2 640x480@30" \
	"[ '$CAPS' = 'video/x-raw,format=YUY2,width=640,height=480,framerate=30/1' ]"

r=$(kernel/vcam/tools/sbs_check $V 90); echo "      $r"
check "90 frames: eye markers + left colour bars / right grey ramp" "[ $? -eq 0 ] && grep -q 'bad=0' <<<'$r'"

t0=$(date +%s.%N)
gst-launch-1.0 -q v4l2src device=$V num-buffers=90 ! "$CAPS" ! videoconvert ! fakesink >/tmp/vcam_gst.txt 2>&1
rc=$?
dt=$(awk -v a=$t0 -v b=$(date +%s.%N) 'BEGIN{printf "%.2f", b-a}')
check "v4l2src negotiates the Pi caps against vcam" "[ $rc -eq 0 ]"
check "90 buffers take ~3 s, i.e. 30 fps ($dt s)" "awk -v d=$dt 'BEGIN{exit !(d>2.6 && d<4.2)}'"

# --- 3: sender script -> RTP/UDP -> receiver pipeline of gst_receiver_node ----
RX='udpsrc port=5600 caps="application/x-rtp, media=video, encoding-name=H264, payload=96" ! rtpjitterbuffer latency=50 ! rtph264depay ! avdec_h264 max-threads=0 ! videoconvert ! video/x-raw,format=BGR'
check "receiver pipeline matches gst_receiver_node.cpp (software path)" \
	"grep -q 'rtph264depay ! \"' ros2_ws/src/gst_receiver/src/gst_receiver_node.cpp && grep -q 'avdec_h264 max-threads=0 ! \"' ros2_ws/src/gst_receiver/src/gst_receiver_node.cpp"

ENC=${H264_ENCODER:-x264enc tune=zerolatency speed-preset=ultrafast key-int-max=30}
if gst-inspect-1.0 --exists "${ENC%% *}" 2>/dev/null && gst-inspect-1.0 --exists rtpjitterbuffer 2>/dev/null; then
	rm -f /tmp/vcam_rx.bgr
	timeout 9 sh -c "gst-launch-1.0 -q $RX ! filesink location=/tmp/vcam_rx.bgr" >/tmp/vcam_rx.txt 2>&1 &
	RXP=$!
	sleep 1
	CAMERA_DEVICE=$V CAMERA_ENABLE_SCRIPT=/nonexistent H264_ENCODER="$ENC" \
		timeout 6 scripts/start_camera_stream.sh 127.0.0.1 5600 >/tmp/vcam_tx.txt 2>&1
	wait $RXP 2>/dev/null
	res=$(python3 - <<'PY'
import numpy as np
a = np.fromfile('/tmp/vcam_rx.bgr', dtype=np.uint8)
fs = 640 * 480 * 3
n = a.size // fs
if n < 30:
    print('frames=%d' % n); raise SystemExit
f = a[:n * fs].reshape(n, 480, 640, 3)[5:].astype(np.int16)
sat = f.max(axis=3) - f.min(axis=3)          # 0 for grey, large for colour bars
left, right = sat[:, :, 8:312].mean(), sat[:, :, 328:632].mean()
print('frames=%d left_sat=%.1f right_sat=%.1f tail=%d' % (n, left, right, a.size % fs))
PY
)
	echo "      $res"
	n=$(sed -n 's/.*frames=\([0-9]*\).*/\1/p' <<<"$res")
	check "decoded >= 100 frames of 640x480 BGR in 6 s (${n:-0})" "[ ${n:-0} -ge 100 ]"
	check "left half is the colour eye, right half the grey eye after H.264" \
		"awk '{split(\$2,l,\"=\"); split(\$3,r,\"=\"); exit !(l[2]>60 && r[2]<15)}' <<<'$res'"
else
	note "no H.264 encoder (${ENC%% *}) or rtpjitterbuffer here; skipped the RTP loopback"
fi

check "module unloads cleanly" "rmmod vcam"
bad_lines=$(dmesg | tail -n +$((DM0+1)) | grep -ciE 'BUG:|WARNING:|Oops|Call Trace|use-after-free|refcount')
check "no kernel BUG/WARNING/Oops in dmesg" "[ $bad_lines -eq 0 ]"

echo "-----"
echo "passed=$PASS failed=$FAIL"
[ $FAIL -eq 0 ]
