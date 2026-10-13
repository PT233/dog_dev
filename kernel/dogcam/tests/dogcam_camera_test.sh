#!/bin/bash
# Real-hardware test of dogcam.ko against a UVC camera that offers YUY2.
#
#   sudo V4L_UTILS=/path/to/v4l-utils/bin ./tests/dogcam_camera_test.sh <usb-dev>
#   e.g. ... ./tests/dogcam_camera_test.sh 1-6
#
# The stock uvcvideo driver is measured first on the same camera and mode and
# serves as the reference.  The camera is then taken away from uvcvideo for
# the test and handed back on exit, whatever happens.  Captured frames are
# only measured (sizes, counts, hashes, statistics) and deleted; no image is
# kept.
set -u
export LC_ALL=C
cd "$(dirname "$0")/.."
REPO="$(cd ../.. && pwd)"
DEV=${1:?usb device, e.g. 1-6}
V4L=${V4L_UTILS:-/usr/bin}
SYS=/sys/bus/usb/devices/$DEV
FRAME=614400	# 640 * 480 * 2
PASS=0; FAIL=0
ok()   { PASS=$((PASS+1)); printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf 'FAIL  %s\n' "$1"; }
note() { printf 'NOTE  %s\n' "$1"; }
check(){ if eval "$2"; then ok "$1"; else bad "$1  [$2]"; fi; }

[ -d "$SYS" ] || { echo "no such usb device: $DEV"; exit 1; }
VID=$(cat $SYS/idVendor); PID=$(cat $SYS/idProduct)
VC=""; VS=""
for i in $SYS/$DEV:*; do
	[ "$(cat $i/bInterfaceClass)" = 0e ] || continue
	[ "$(cat $i/bInterfaceSubClass)" = 01 ] && VC=$(basename $i)
	[ "$(cat $i/bInterfaceSubClass)" = 02 ] && [ -z "$VS" ] && VS=$(basename $i)
done
[ -n "$VC" ] || { echo "$DEV is not a UVC camera"; exit 1; }
drv_of() { basename "$(readlink $SYS/$1/driver 2>/dev/null)" 2>/dev/null; }
ORIG=$(drv_of $VC)
echo "camera $VID:$PID control=$VC streaming=$VS (was bound to: ${ORIG:-nothing})"

restore() {
	pkill -f "stream-mmap" 2>/dev/null
	pkill -f 'udpsrc port=5600' 2>/dev/null
	pkill -f start_camera_stream.sh 2>/dev/null
	sleep 0.3
	rmmod dogcam 2>/dev/null
	rm -f /tmp/dogcam_*.raw /tmp/dogcam_rx.bgr /tmp/dogcam_*.txt
	if [ "$ORIG" = uvcvideo ] && [ "$(drv_of $VC)" != uvcvideo ]; then
		echo -n $VC > /sys/bus/usb/drivers/uvcvideo/bind 2>/dev/null
	fi
	echo "restored: $VC is bound to $(drv_of $VC)"
}
trap restore EXIT

make -s ${MAKE_ARGS:-} 2>&1 | grep -E 'warning:|error' | grep -v 'compiler differs' && { echo "build has warnings"; exit 1; }
[ -x tools/ctrl_query ] || gcc -O2 -Wall -o tools/ctrl_query tools/ctrl_query.c || exit 1

node_of() {	# video node whose parent is the control interface (first one)
	for d in $(ls -d /sys/class/video4linux/video* | sort -V); do
		[ "$(basename "$(readlink -f $d/device)")" = "$VC" ] && { echo /dev/$(basename $d); return; }
	done
}
stopped() { dmesg | tail -n +$((DM0+1)) | grep 'stream stopped' | tail -1; }
num() { sed -n "s/.* \([0-9]*\) $2.*/\1/p" <<<"$1"; }
get() { sed -n "s/.*[ ]$2=\([^ ]*\).*/\1/p" <<<" $1"; }

# capture <node> <count> <file>  ->  "n=.. same=.. md5=.. ymean=.. secs=.. fps=.."
capture() {
	local t0 v
	t0=$(date +%s.%N)
	$V4L/v4l2-ctl -d $1 --stream-mmap --stream-count=$2 --stream-to=$3 >/dev/null 2>&1
	local secs=$(awk -v a=$t0 -v b=$(date +%s.%N) 'BEGIN{printf "%.2f", b-a}')
	v=$($V4L/v4l2-ctl -d $1 --stream-mmap --stream-count=45 --verbose 2>&1 | grep 'cap dqbuf')
	# frames / time span from the buffer timestamps; averaging the per-frame
	# "fps:" column would be biased when the camera varies its frame interval
	local fps=$(sed -n 's/.* ts: *\([0-9.]*\).*/\1/p' <<<"$v" | tail -30 | awk 'NR==1{a=$1} {b=$1;n++} END{if(n>1 && b>a) printf "%.1f", (n-1)/(b-a); else print 0}')
	local gaps=$(sed -n 's/.*seq: *\([0-9]*\).*/\1/p' <<<"$v" | awk 'NR>1 && $1!=p+1{g++} {p=$1} END{print g+0}')
	python3 - "$3" "$secs" "$fps" "$gaps" <<'PY'
import sys, hashlib, collections
import numpy as np
a = np.fromfile(sys.argv[1], dtype=np.uint8)
fs = 614400
n = a.size // fs
f = a[:n * fs].reshape(n, fs) if n else a.reshape(0, fs)
same = sum(int(np.array_equal(f[i], f[i - 1])) for i in range(1, n))
h = collections.Counter(hashlib.md5(x.tobytes()).hexdigest()[:12] for x in f)
md5 = h.most_common(1)[0][0] if n else 'none'
ymean = float(f[-1][0::2].mean()) if n else 0.0
print('n=%d tail=%d same=%d md5=%s ymean=%.1f secs=%s fps=%s gaps=%s' %
      (n, a.size % fs, same, md5, ymean, sys.argv[2], sys.argv[3], sys.argv[4]))
PY
	rm -f $3
}

# --- reference: the stock driver, same camera, same mode -----------------------
REF=""
if [ "$ORIG" = uvcvideo ]; then
	RV=$(node_of)
	$V4L/v4l2-ctl -d $RV --set-fmt-video=width=640,height=480,pixelformat=YUYV >/dev/null 2>&1
	REF=$(capture $RV 60 /tmp/dogcam_ref.raw)
	echo "      uvcvideo: $REF"
	echo -n $VC > /sys/bus/usb/drivers/uvcvideo/unbind
fi

DM0=$(dmesg | wc -l)
modprobe videobuf2-vmalloc
insmod ./dogcam.ko || { echo "insmod failed"; exit 1; }
echo "$VID $PID" > /sys/bus/usb/drivers/dogcam/new_id
sleep 1

check "dogcam bound to the VideoControl interface" "[ \"\$(drv_of $VC)\" = dogcam ]"
check "dogcam claimed the VideoStreaming interface" "[ \"\$(drv_of $VS)\" = dogcam ]"
V=$(node_of)
check "video node registered ($V)" "[ -c '$V' ]"

$V4L/v4l2-ctl -d $V --all >/tmp/dogcam_all.txt 2>&1
check "QUERYCAP reports driver dogcam" "grep -q 'Driver name *: dogcam' /tmp/dogcam_all.txt"
check "default format is YUYV 640x480" \
	"grep -q 'Width/Height *: 640/480' /tmp/dogcam_all.txt && grep -q YUYV /tmp/dogcam_all.txt"
check "frame interval reported as exactly 30/1" "grep -q '(30/1)' /tmp/dogcam_all.txt"
$V4L/v4l2-ctl -d $V --list-formats-ext >/tmp/dogcam_fmts.txt 2>&1
check "frame sizes come from the camera's descriptors (640x480 listed)" "grep -q 'Discrete 640x480' /tmp/dogcam_fmts.txt"
check "30 fps interval parsed from the descriptors" "grep -q '30.000 fps' /tmp/dogcam_fmts.txt"

# --- streaming, compared with the reference -----------------------------------
DOG=$(capture $V 60 /tmp/dogcam_a.raw)
echo "      dogcam:   $DOG"
check "60 frames, each exactly 640*480*2 bytes" "[ \"\$(get '$DOG' n)\" = 60 ] && [ \"\$(get '$DOG' tail)\" = 0 ]"
check "sequence numbers are consecutive (gaps=$(get "$DOG" gaps))" "[ \"\$(get '$DOG' gaps)\" = 0 ]"
if [ -n "$REF" ]; then
	if [ "$(get "$REF" same)" -ge 30 ]; then
		# the camera is showing a still picture: both drivers must deliver the same bytes
		check "frame is byte-identical to uvcvideo's (md5 $(get "$DOG" md5))" \
			"[ \"\$(get '$DOG' md5)\" = \"\$(get '$REF' md5)\" ]"
	else
		check "live scene: mean luma close to uvcvideo's ($(get "$DOG" ymean) vs $(get "$REF" ymean))" \
			"awk -v a=$(get "$DOG" ymean) -v b=$(get "$REF" ymean) 'BEGIN{d=a-b; if(d<0)d=-d; exit !(d<20)}'"
	fi
	check "frame rate matches uvcvideo's within 35% ($(get "$DOG" fps) vs $(get "$REF" fps) fps)" \
		"awk -v a=$(get "$DOG" fps) -v b=$(get "$REF" fps) 'BEGIN{exit !(a>b*0.65 && a<b*1.35)}'"
	check "60 frames take as long as with uvcvideo within 35% ($(get "$DOG" secs) vs $(get "$REF" secs) s)" \
		"awk -v a=$(get "$DOG" secs) -v b=$(get "$REF" secs) 'BEGIN{exit !(a>b*0.65 && a<b*1.35)}'"
else
	note "uvcvideo was not bound to this camera, no reference to compare against"
fi
s=$(stopped); echo "      ${s##*] }"
inc=$(num "$s" incomplete)
check "driver stats: at most the first frame incomplete (${inc:-?})" "[ ${inc:-9} -le 2 ]"
check "driver stats: no isochronous packet errors" "[ \"\$(num '$s' 'packet errors')\" = 0 ]"

# --- the robot dog's own capture pipeline, on the real camera ------------------
CAPS='video/x-raw,format=YUY2,width=640,height=480,framerate=30/1'
gst-launch-1.0 -q v4l2src device=$V num-buffers=45 ! "$CAPS" ! videoconvert ! fakesink >/tmp/dogcam_gst.txt 2>&1
check "v4l2src with start_camera_stream.sh's caps streams from dogcam" "[ $? -eq 0 ]"

ENC=${H264_ENCODER:-x264enc tune=zerolatency speed-preset=ultrafast key-int-max=30}
if gst-inspect-1.0 --exists "${ENC%% *}" 2>/dev/null; then
	RX='udpsrc port=5600 caps="application/x-rtp, media=video, encoding-name=H264, payload=96" ! rtpjitterbuffer latency=50 ! rtph264depay ! avdec_h264 max-threads=0 ! videoconvert ! video/x-raw,format=BGR'
	timeout 9 sh -c "gst-launch-1.0 -q $RX ! filesink location=/tmp/dogcam_rx.bgr" >/dev/null 2>&1 &
	RXP=$!
	sleep 1
	CAMERA_DEVICE=$V CAMERA_ENABLE_SCRIPT=/nonexistent H264_ENCODER="$ENC" \
		timeout 6 "$REPO/scripts/start_camera_stream.sh" 127.0.0.1 5600 >/dev/null 2>&1
	wait $RXP 2>/dev/null
	n=$(( $(stat -c %s /tmp/dogcam_rx.bgr 2>/dev/null || echo 0) / 921600 ))
	rm -f /tmp/dogcam_rx.bgr
	check "camera -> start_camera_stream.sh -> H.264/RTP -> receiver pipeline: $n frames decoded" "[ $n -ge 40 ]"
else
	note "no H.264 encoder (${ENC%% *}) here; skipped the RTP loopback"
fi

# --- V4L2 compliance ------------------------------------------------------------
$V4L/v4l2-compliance -d $V -s >/tmp/dogcam_comp.txt 2>&1
note "$(grep '^Total' /tmp/dogcam_comp.txt)"
nfail=$(grep -cE 'test .*: FAIL' /tmp/dogcam_comp.txt)
if [ "$nfail" -eq 0 ]; then
	ok "v4l2-compliance: no failures"
elif [ "$nfail" -eq 1 ] && grep -qE 'REQBUFS/CREATE_BUFS/QUERYBUF: FAIL' /tmp/dogcam_comp.txt; then
	ok "v4l2-compliance: only the old-tool/6.8 reserved[] skew (see kernel/vcam/tools/crbufs_probe)"
else
	bad "v4l2-compliance: unexpected failures"
	grep -B3 -E 'test .*: FAIL' /tmp/dogcam_comp.txt | grep -E 'fail:|FAIL' | sed 's/^[[:space:]]*/      /'
fi

# --- UVCIOC_CTRL_QUERY pass-through ---------------------------------------------
PU=$(python3 - "$SYS/descriptors" <<'PY'
import sys
b = open(sys.argv[1], 'rb').read()
i = 0
while i + 2 < len(b) and b[i]:
    if b[i + 1] == 0x24 and b[i + 2] == 0x05:      # CS_INTERFACE, VC_PROCESSING_UNIT
        print(b[i + 3]); break
    i += b[i]
PY
)
if [ -n "$PU" ]; then
	cur=$(./tools/ctrl_query $V $PU 0x02 0x81 2 2>&1)	# PU_BRIGHTNESS GET_CUR
	mn=$(./tools/ctrl_query $V $PU 0x02 0x82 2 2>&1)
	mx=$(./tools/ctrl_query $V $PU 0x02 0x83 2 2>&1)
	echo "      processing unit $PU brightness: cur=[$cur] min=[$mn] max=[$mx]"
	check "UVCIOC_CTRL_QUERY reaches the VideoControl interface (brightness within min..max)" \
		"python3 -c \"import sys; v=lambda s: int.from_bytes(bytes.fromhex(s.replace(' ','')), 'little', signed=True); c,a,b=v('$cur'),v('$mn'),v('$mx'); sys.exit(not (a<=c<=b and a<b))\""
else
	note "camera has no processing unit; skipped the control query check"
fi

# --- start/stop cycles and unbind while streaming ------------------------------
badc=0
for i in $(seq 20); do
	$V4L/v4l2-ctl -d $V --stream-mmap --stream-count=5 --stream-to=/tmp/dogcam_c.raw >/dev/null 2>&1
	[ "$(stat -c %s /tmp/dogcam_c.raw 2>/dev/null)" = $((5*FRAME)) ] || badc=$((badc+1))
done
rm -f /tmp/dogcam_c.raw
check "20x STREAMON / 5 frames / STREAMOFF (renegotiate, alt setting, URBs)" "[ $badc -eq 0 ]"

$V4L/v4l2-ctl -d $V --stream-mmap --stream-count=100000 >/dev/null 2>&1 &
CP=$!
sleep 1.5
echo -n $VC > /sys/bus/usb/drivers/dogcam/unbind
for i in $(seq 30); do kill -0 $CP 2>/dev/null || break; sleep 0.1; done
if kill -0 $CP 2>/dev/null; then bad "streaming client exits after unbind"; kill -9 $CP
else ok "streaming client exits after unbind"; fi
check "streaming interface released on unbind" "[ -z \"\$(drv_of $VS)\" ]"
echo -n $VC > /sys/bus/usb/drivers/dogcam/bind
sleep 1
V=$(node_of)
check "rebind brings the node back ($V)" "[ -c '$V' ]"
$V4L/v4l2-ctl -d $V --stream-mmap --stream-count=30 --stream-to=/tmp/dogcam_c.raw >/dev/null 2>&1
check "streams again after rebind" "[ \"\$(stat -c %s /tmp/dogcam_c.raw 2>/dev/null)\" = $((30*FRAME)) ]"
rm -f /tmp/dogcam_c.raw

check "module unloads cleanly" "rmmod dogcam"
bad_lines=$(dmesg | tail -n +$((DM0+1)) | grep -ciE 'BUG:|WARNING:|Oops|Call Trace|use-after-free|refcount|scheduling while atomic')
check "no kernel BUG/WARNING/Oops in dmesg" "[ $bad_lines -eq 0 ]"

echo "-----"
echo "passed=$PASS failed=$FAIL"
[ $FAIL -eq 0 ]
