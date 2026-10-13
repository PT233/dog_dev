#!/bin/bash
# Regression test for vcam.ko.  Run as root on the machine under test:
#   sudo V4L_UTILS=/path/to/v4l-utils/bin ./tests/vcam_test.sh
# Needs v4l2-ctl, v4l2-compliance, media-ctl (V4L_UTILS, default /usr/bin).
set -u
export LC_ALL=C
cd "$(dirname "$0")/.."
V4L=${V4L_UTILS:-/usr/bin}
PASS=0; FAIL=0
ok()   { PASS=$((PASS+1)); printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf 'FAIL  %s\n' "$1"; }
check(){ if eval "$2"; then ok "$1"; else bad "$1  [$2]"; fi; }
note()  { printf 'NOTE  %s\n' "$1"; }

cleanup() { pkill -f qbuf_race 2>/dev/null; rmmod vcam 2>/dev/null; }
trap cleanup EXIT

make -s ${MAKE_ARGS:-} 2>&1 | grep -E 'warning:|error' | grep -v 'compiler differs' && { echo "build has warnings"; exit 1; }
for t in qbuf_race crbufs_probe sbs_check; do
	[ -x tools/$t ] || gcc -O2 -Wall -o tools/$t tools/$t.c || exit 1
done

find_node() {
	for d in /sys/class/video4linux/*; do
		[ "$(cat $d/name 2>/dev/null)" = vcam ] && { echo /dev/$(basename $d); return; }
	done
}
race() { ./tools/qbuf_race "$@"; }
field() { sed -n "s/.*[ ]$2=\([0-9.]*\).*/\1/p" <<<"$1"; }

DM0=$(dmesg | wc -l)
modprobe videobuf2-vmalloc && insmod ./vcam.ko || { echo "insmod failed"; exit 1; }
sleep 0.5
V=$(find_node)
check "probe: video node registered ($V)" "[ -c '$V' ]"
MEDIA=/dev/$(ls /sys/bus/platform/devices/vcam | grep '^media')

topo=$($V4L/media-ctl -d $MEDIA -p 2>&1)
check "media graph: sensor -> capture, link ENABLED,IMMUTABLE" \
	"grep -q '\"vcam\":0 \[ENABLED,IMMUTABLE\]' <<<'$topo'"
check "media graph: sensor pad format YUYV8_2X8/640x480" \
	"grep -q 'YUYV8_2X8/640x480' <<<'$topo'"
check "media graph: sensor entity function is Sensor" "grep -q 'subtype Sensor' <<<'$topo'"

# --- default mode is the dog's stereo camera: YUYV 640x480, eyes side by side ---
rsb=$(./tools/sbs_check $V 60); echo "      $rsb"
check "default pattern is side-by-side stereo with eye markers" "grep -q 'layout=sbs 640x480 frames=60 bad=0' <<<'$rsb'"

# --- v4l2-compliance ---------------------------------------------------------
comp=$($V4L/v4l2-compliance -d $V -s 2>&1)
fails=$(grep -E 'test .*: FAIL' <<<"$comp" | sed 's/^[[:space:]]*//')
nfail=$(grep -c ': FAIL' <<<"$fails")
summary=$(grep '^Total' <<<"$comp")
note "$summary"
if [ "$nfail" -eq 0 ]; then
	ok "v4l2-compliance: no failures"
elif [ "$nfail" -eq 1 ] && grep -q 'REQBUFS/CREATE_BUFS/QUERYBUF' <<<"$fails"; then
	if ./tools/crbufs_probe $V >/tmp/crbufs.txt; then
		note "known ABI skew, not a driver bug: $(cat /tmp/crbufs.txt)"
		ok "v4l2-compliance: only failure is the old-tool/6.8 reserved[] skew"
	else
		bad "v4l2-compliance: REQBUFS failure is NOT the known ABI skew"
	fi
else
	bad "v4l2-compliance: unexpected failures: $fails"
fi

# --- controls reach the pixels ------------------------------------------------
$V4L/v4l2-ctl -d $V -c brightness=200,test_pattern=2 >/dev/null 2>&1
got=$($V4L/v4l2-ctl -d $V --get-ctrl brightness,test_pattern 2>&1 | tr '\n' ' ')
check "controls: set/get roundtrip ($got)" \
	"grep -q 'brightness: 200' <<<'$got' && grep -q 'test_pattern: 2' <<<'$got'"
rm -f /tmp/vcam_f.yuv
$V4L/v4l2-ctl -d $V --stream-mmap --stream-count=3 --stream-to=/tmp/vcam_f.yuv >/dev/null 2>&1
y=$(od -An -tu1 -j16 -N4 /tmp/vcam_f.yuv | tr -s ' ')
check "controls: flat gray + brightness 200 gives Y=200 ($y)" "[ '$y' = ' 200 128 200 128' ]"
$V4L/v4l2-ctl -d $V -c brightness=128,test_pattern=0 >/dev/null 2>&1

# --- formats and frame rate ---------------------------------------------------
for s in 320x240 640x480 1280x720; do
	$V4L/v4l2-ctl -d $V --set-fmt-video=width=${s%x*},height=${s#*x},pixelformat=YUYV >/dev/null 2>&1
	rm -f /tmp/vcam_f.yuv
	$V4L/v4l2-ctl -d $V --stream-mmap --stream-count=1 --stream-to=/tmp/vcam_f.yuv >/dev/null 2>&1
	want=$(( ${s%x*} * ${s#*x} * 2 ))
	check "format $s: one frame is $want bytes" "[ \"\$(stat -c %s /tmp/vcam_f.yuv)\" = $want ]"
done
$V4L/v4l2-ctl -d $V --set-fmt-video=width=640,height=480,pixelformat=YUYV >/dev/null 2>&1
check "S_FMT with an unsupported size snaps to a supported one" \
	"$V4L/v4l2-ctl -d $V --set-fmt-video=width=1000,height=700,pixelformat=YUYV 2>&1 | grep -q . ; \
	 $V4L/v4l2-ctl -d $V --get-fmt-video | grep -q 'Width/Height      : 1280/720'"
$V4L/v4l2-ctl -d $V --set-fmt-video=width=640,height=480,pixelformat=YUYV >/dev/null 2>&1

$V4L/v4l2-ctl -d $V --set-parm=15 >/dev/null 2>&1
r=$(race $V hold 30 0)
check "frame rate 15 fps honoured ($(field "$r" rate) fps)" \
	"awk -v v=$(field "$r" rate) 'BEGIN{exit !(v>13.5 && v<16.5)}'"
$V4L/v4l2-ctl -d $V --set-parm=30 >/dev/null 2>&1

# --- the DMABUF re-queue bug and its two fixes --------------------------------
rb=$(race $V bug 40 150 -b 4); echo "      $rb"
rc=$(race $V copy 40 150 -b 4); echo "      $rc"
rh=$(race $V hold 40 150 -b 4); echo "      $rh"
check "bug mode reproduces overwritten frames (corrupted=$(field "$rb" corrupted))" \
	"[ $(field "$rb" corrupted) -gt 0 ]"
check "copy fix: no corrupted/torn frames" \
	"[ $(field "$rc" corrupted) -eq 0 ] && [ $(field "$rc" torn) -eq 0 ]"
check "hold fix: no corrupted/torn frames" \
	"[ $(field "$rh" corrupted) -eq 0 ] && [ $(field "$rh" torn) -eq 0 ]"
rx=$(race $V copy 30 20 -b 4 -x); echo "      $rx"
check "EXPBUF dma-buf mapping delivers consistent frames" \
	"[ $(field "$rx" frames) -eq 30 ] && [ $(field "$rx" corrupted) -eq 0 ] && [ $(field "$rx" torn) -eq 0 ]"
rs=$(race $V copy 20 10 -s 1280x720); echo "      $rs"
check "1280x720 streaming through the race tool" "[ $(field "$rs" frames) -eq 20 ] && [ $(field "$rs" torn) -eq 0 ]"

# --- unbind while streaming, then rebind --------------------------------------
race $V copy 100000 0 -b 4 >/tmp/vcam_unbind.txt 2>&1 &
RP=$!
sleep 1
echo vcam > /sys/bus/platform/drivers/vcam/unbind
for i in $(seq 30); do kill -0 $RP 2>/dev/null || break; sleep 0.1; done
if kill -0 $RP 2>/dev/null; then bad "streaming client exits after unbind"; kill -9 $RP
else ok "streaming client exits after unbind"; fi
check "video node gone after unbind" "[ -z \"\$(find_node)\" ]"
echo vcam > /sys/bus/platform/drivers/vcam/bind
sleep 0.5
V=$(find_node)
check "rebind brings the node back ($V)" "[ -c '$V' ]"
rr=$(race $V copy 15 0)
check "streams again after rebind" "[ $(field "$rr" frames) -eq 15 ]"

# --- repeated cycles ----------------------------------------------------------
bad_cycle=0
for i in $(seq 20); do
	race $V copy 5 0 >/dev/null 2>&1 || bad_cycle=$((bad_cycle+1))
	echo vcam > /sys/bus/platform/drivers/vcam/unbind
	echo vcam > /sys/bus/platform/drivers/vcam/bind
	sleep 0.1
	V=$(find_node)
done
check "20x (stream, unbind, bind) without failures" "[ $bad_cycle -eq 0 ]"

check "module unloads cleanly" "rmmod vcam"
bad_lines=$(dmesg | tail -n +$((DM0+1)) | grep -ciE 'BUG:|WARNING:|Oops|lockdep|Call Trace|use-after-free|refcount')
check "no kernel BUG/WARNING/Oops in dmesg" "[ $bad_lines -eq 0 ]"

echo "-----"
echo "passed=$PASS failed=$FAIL"
[ $FAIL -eq 0 ]
