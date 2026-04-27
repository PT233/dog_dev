#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

PI_SSH="${PI_SSH:-ubuntu@192.168.137.100}"
REMOTE_WS="${REMOTE_WS:-/home/ubuntu/ros2_ws}"
REMOTE_PROJECT="${REMOTE_PROJECT:-/home/ubuntu/desktop_tracking_robot}"
RPI_ROS_DOMAIN_ID="${RPI_ROS_DOMAIN_ID:-42}"
WSL_TARGET_IP="${WSL_TARGET_IP:-}"
PREBUILT_TAR=""
RUN_SMOKE=1
RUN_HARDWARE_SMOKE=0
START_AFTER_DEPLOY=0
ENABLE_BOOT_SERVICES=1
DISABLE_BOOT_SERVICES=0
REMOTE_USER=""

PI_PACKAGES=(robot_interfaces uart_bridge robot_bringup)
RPI_SCRIPTS=(start_camera_stream.sh rpi_start_camera.sh rpi_start_ros.sh ros2_network_env.sh test_uart_rpi.py)

usage() {
  cat <<'EOF'
Usage:
  scripts/rpi_one_click_deploy.sh [options]

Options:
  --pi USER@HOST             SSH target. Default: ubuntu@192.168.137.100
  --remote-ws PATH           Remote ROS workspace. Default: /home/ubuntu/ros2_ws
  --remote-project PATH      Remote project dir for scripts. Default: /home/ubuntu/desktop_tracking_robot
  --domain-id ID             ROS_DOMAIN_ID to write on the Pi. Default: 42
  --wsl-ip IP                UDP video target IP. Default: derived from route to the Pi
  --prebuilt-tar PATH        Deploy an ARM64 install tarball instead of building on the Pi
  --boot-services            Install and enable boot-time systemd services. Default
  --no-boot-services         Do not install or change boot-time systemd services
  --disable-boot-services    Stop and disable existing boot-time systemd services
  --skip-smoke               Skip non-hardware smoke checks
  --hardware-smoke           Also probe /dev/ttyAMA0 and /dev/video0 by launching briefly
  --start                    Start camera stream and rpi_stack immediately after deploy
  -h, --help                 Show this help

Environment overrides:
  PI_SSH, REMOTE_WS, REMOTE_PROJECT, RPI_ROS_DOMAIN_ID, WSL_TARGET_IP
  ROBOT_DDS_INTERFACE, ROBOT_DDS_PEERS
EOF
}

die() {
  echo "ERROR: $*" >&2
  exit 1
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "Missing required command: $1"
}

derive_wsl_ip() {
  local host="${PI_SSH#*@}"
  host="${host%%:*}"
  ip route get "$host" 2>/dev/null | awk '
    {
      for (i = 1; i <= NF; i++) {
        if ($i == "src") {
          print $(i + 1)
          exit
        }
      }
    }'
}

remote_run() {
  ssh "$PI_SSH" \
    "REMOTE_WS='$REMOTE_WS' REMOTE_PROJECT='$REMOTE_PROJECT' RPI_ROS_DOMAIN_ID='$RPI_ROS_DOMAIN_ID' WSL_TARGET_IP='$WSL_TARGET_IP' REMOTE_USER='$REMOTE_USER' ROBOT_DDS_INTERFACE='${ROBOT_DDS_INTERFACE:-}' ROBOT_DDS_PEERS='${ROBOT_DDS_PEERS:-}' bash -s"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --pi)
      PI_SSH="$2"
      shift 2
      ;;
    --remote-ws)
      REMOTE_WS="$2"
      shift 2
      ;;
    --remote-project)
      REMOTE_PROJECT="$2"
      shift 2
      ;;
    --domain-id)
      RPI_ROS_DOMAIN_ID="$2"
      shift 2
      ;;
    --wsl-ip)
      WSL_TARGET_IP="$2"
      shift 2
      ;;
    --prebuilt-tar)
      PREBUILT_TAR="$2"
      shift 2
      ;;
    --boot-services)
      ENABLE_BOOT_SERVICES=1
      DISABLE_BOOT_SERVICES=0
      shift
      ;;
    --no-boot-services)
      ENABLE_BOOT_SERVICES=0
      shift
      ;;
    --disable-boot-services)
      ENABLE_BOOT_SERVICES=0
      DISABLE_BOOT_SERVICES=1
      shift
      ;;
    --skip-smoke)
      RUN_SMOKE=0
      shift
      ;;
    --hardware-smoke)
      RUN_HARDWARE_SMOKE=1
      shift
      ;;
    --start)
      START_AFTER_DEPLOY=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "Unknown option: $1"
      ;;
  esac
done

need_cmd ssh
need_cmd rsync

if [[ -n "$PREBUILT_TAR" && ! -f "$PREBUILT_TAR" ]]; then
  die "Prebuilt tarball not found: $PREBUILT_TAR"
fi

if [[ -z "$WSL_TARGET_IP" ]]; then
  WSL_TARGET_IP="$(derive_wsl_ip || true)"
  WSL_TARGET_IP="${WSL_TARGET_IP:-192.168.137.1}"
fi

if [[ "$PI_SSH" == *@* ]]; then
  REMOTE_USER="${PI_SSH%@*}"
else
  REMOTE_USER="$(ssh -o BatchMode=yes -o ConnectTimeout=5 "$PI_SSH" 'id -un')"
fi

echo "Deploy target: $PI_SSH"
echo "Remote workspace: $REMOTE_WS"
echo "Remote project: $REMOTE_PROJECT"
echo "Remote service user: $REMOTE_USER"
echo "ROS_DOMAIN_ID: $RPI_ROS_DOMAIN_ID"
echo "Video target: $WSL_TARGET_IP:5600"

ssh -o BatchMode=yes -o ConnectTimeout=5 "$PI_SSH" \
  "uname -m; test -f /opt/ros/jazzy/setup.bash; command -v colcon >/dev/null; command -v gst-launch-1.0 >/dev/null"

remote_run <<'REMOTE'
set -euo pipefail
mkdir -p "$REMOTE_WS/src" "$REMOTE_PROJECT/scripts" "$REMOTE_PROJECT/logs"
if ! grep -q '^# BEGIN desktop_tracking_robot ROS 2$' "$HOME/.bashrc" 2>/dev/null; then
  cat >> "$HOME/.bashrc" <<EOF

# BEGIN desktop_tracking_robot ROS 2
export ROS_DOMAIN_ID=$RPI_ROS_DOMAIN_ID
export ROS_LOCALHOST_ONLY=0
export ROBOT_DDS_ROLE=rpi
export ROBOT_WSL_IP=$WSL_TARGET_IP
export ROBOT_DDS_INTERFACE=${ROBOT_DDS_INTERFACE:-eth0}
if [ -n "${ROBOT_DDS_PEERS:-}" ]; then
  export ROBOT_DDS_PEERS="$ROBOT_DDS_PEERS"
fi
export ROBOT_DDS_QUIET=1
[ -f "$REMOTE_PROJECT/scripts/ros2_network_env.sh" ] && source "$REMOTE_PROJECT/scripts/ros2_network_env.sh"
unset ROBOT_DDS_QUIET
# END desktop_tracking_robot ROS 2
EOF
else
  sed -i '/^# BEGIN desktop_tracking_robot ROS 2$/,/^# END desktop_tracking_robot ROS 2$/d' "$HOME/.bashrc"
  cat >> "$HOME/.bashrc" <<EOF

# BEGIN desktop_tracking_robot ROS 2
export ROS_DOMAIN_ID=$RPI_ROS_DOMAIN_ID
export ROS_LOCALHOST_ONLY=0
export ROBOT_DDS_ROLE=rpi
export ROBOT_WSL_IP=$WSL_TARGET_IP
export ROBOT_DDS_INTERFACE=${ROBOT_DDS_INTERFACE:-eth0}
if [ -n "${ROBOT_DDS_PEERS:-}" ]; then
  export ROBOT_DDS_PEERS="$ROBOT_DDS_PEERS"
fi
export ROBOT_DDS_QUIET=1
[ -f "$REMOTE_PROJECT/scripts/ros2_network_env.sh" ] && source "$REMOTE_PROJECT/scripts/ros2_network_env.sh"
unset ROBOT_DDS_QUIET
# END desktop_tracking_robot ROS 2
EOF
fi
REMOTE

for pkg in "${PI_PACKAGES[@]}"; do
  [[ -d "$ROOT_DIR/ros2_ws/src/$pkg" ]] || die "Missing package: ros2_ws/src/$pkg"
  rsync -az --delete \
    --exclude build --exclude install --exclude log --exclude __pycache__ \
    "$ROOT_DIR/ros2_ws/src/$pkg/" "$PI_SSH:$REMOTE_WS/src/$pkg/"
done

for script in "${RPI_SCRIPTS[@]}"; do
  [[ -f "$ROOT_DIR/scripts/$script" ]] || die "Missing script: scripts/$script"
  rsync -az "$ROOT_DIR/scripts/$script" "$PI_SSH:$REMOTE_PROJECT/scripts/$script"
done

remote_run <<'REMOTE'
set -euo pipefail
chmod +x "$REMOTE_PROJECT"/scripts/*.sh
REMOTE

if [[ -n "$PREBUILT_TAR" ]]; then
  rsync -az "$PREBUILT_TAR" "$PI_SSH:/tmp/ros2_ws_install_rpi_aarch64.tgz"
  remote_run <<'REMOTE'
set -euo pipefail
rm -rf "$REMOTE_WS/install"
mkdir -p "$REMOTE_WS/install"
tar -xzf /tmp/ros2_ws_install_rpi_aarch64.tgz -C "$REMOTE_WS/install"
REMOTE
else
  remote_run <<'REMOTE'
set -euo pipefail
set +u
source /opt/ros/jazzy/setup.bash
set -u
cd "$REMOTE_WS"
colcon build \
  --packages-select robot_interfaces uart_bridge robot_bringup \
  --allow-overriding robot_interfaces \
  --parallel-workers 1 \
  --cmake-args -DBUILD_TESTING=OFF
REMOTE
fi

if [[ "$RUN_SMOKE" -eq 1 ]]; then
  remote_run <<'REMOTE'
set -euo pipefail
set +u
source /opt/ros/jazzy/setup.bash
source "$REMOTE_WS/install/setup.bash"
set -u
export ROBOT_DDS_ROLE=rpi
export ROBOT_WSL_IP="$WSL_TARGET_IP"
source "$REMOTE_PROJECT/scripts/ros2_network_env.sh"
ros2 interface show robot_interfaces/msg/TargetInfo >/dev/null
ros2 launch --show-args robot_bringup rpi_stack.launch.py >/dev/null
test -x "$REMOTE_PROJECT/scripts/start_camera_stream.sh"
REMOTE
fi

if [[ "$RUN_HARDWARE_SMOKE" -eq 1 ]]; then
  remote_run <<'REMOTE'
set -euo pipefail
set +u
source /opt/ros/jazzy/setup.bash
source "$REMOTE_WS/install/setup.bash"
set -u
export ROBOT_DDS_ROLE=rpi
export ROBOT_WSL_IP="$WSL_TARGET_IP"
source "$REMOTE_PROJECT/scripts/ros2_network_env.sh"
test -e /dev/ttyAMA0
test -e /dev/video0
timeout 6s ros2 launch robot_bringup rpi_stack.launch.py || true
timeout 6s "$REMOTE_PROJECT/scripts/start_camera_stream.sh" "$WSL_TARGET_IP" 5600 || true
REMOTE
fi

if [[ "$DISABLE_BOOT_SERVICES" -eq 1 ]]; then
  remote_run <<'REMOTE'
set -euo pipefail
if ! sudo -n true 2>/dev/null; then
  echo "ERROR: passwordless sudo is required to disable systemd boot services." >&2
  exit 3
fi
sudo systemctl disable --now \
  desktop-tracking-camera.service \
  desktop-tracking-rpi-stack.service 2>/dev/null || true
sudo rm -f \
  /etc/systemd/system/desktop-tracking-camera.service \
  /etc/systemd/system/desktop-tracking-rpi-stack.service
sudo systemctl daemon-reload
REMOTE
fi

if [[ "$ENABLE_BOOT_SERVICES" -eq 1 ]]; then
  remote_run <<'REMOTE'
set -euo pipefail
if ! sudo -n true 2>/dev/null; then
  echo "ERROR: passwordless sudo is required to install systemd boot services." >&2
  echo "Either configure sudo for $REMOTE_USER or rerun with --no-boot-services." >&2
  exit 3
fi

mkdir -p "$REMOTE_PROJECT/logs/ros"

sudo tee /etc/systemd/system/desktop-tracking-camera.service >/dev/null <<EOF
[Unit]
Description=Desktop Tracking Robot camera stream
Wants=network-online.target
After=network-online.target

[Service]
Type=simple
User=$REMOTE_USER
SupplementaryGroups=video
WorkingDirectory=$REMOTE_PROJECT
Environment=ROS_DOMAIN_ID=$RPI_ROS_DOMAIN_ID
Environment=ROS_LOCALHOST_ONLY=0
Environment=CAMERA_DEVICE=/dev/video0
ExecStartPre=/usr/bin/test -e /dev/video0
ExecStart=$REMOTE_PROJECT/scripts/start_camera_stream.sh $WSL_TARGET_IP 5600
Restart=always
RestartSec=5
TimeoutStopSec=10

[Install]
WantedBy=multi-user.target
EOF

sudo tee /etc/systemd/system/desktop-tracking-rpi-stack.service >/dev/null <<EOF
[Unit]
Description=Desktop Tracking Robot Raspberry Pi ROS stack
Wants=network-online.target
After=network-online.target

[Service]
Type=simple
User=$REMOTE_USER
SupplementaryGroups=dialout
WorkingDirectory=$REMOTE_WS
Environment=ROS_DOMAIN_ID=$RPI_ROS_DOMAIN_ID
Environment=ROS_LOCALHOST_ONLY=0
Environment=ROBOT_DDS_ROLE=rpi
Environment=ROBOT_WSL_IP=$WSL_TARGET_IP
Environment=ROBOT_DDS_INTERFACE=${ROBOT_DDS_INTERFACE:-eth0}
Environment=ROBOT_DDS_PEERS=${ROBOT_DDS_PEERS:-$WSL_TARGET_IP}
Environment=ROS_LOG_DIR=$REMOTE_PROJECT/logs/ros
ExecStartPre=/usr/bin/test -e /dev/ttyAMA0
ExecStart=/usr/bin/bash -lc 'source $REMOTE_PROJECT/scripts/ros2_network_env.sh && set +u && source /opt/ros/jazzy/setup.bash && source $REMOTE_WS/install/setup.bash && exec ros2 launch robot_bringup rpi_stack.launch.py'
KillSignal=SIGINT
Restart=always
RestartSec=5
TimeoutStopSec=15

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable \
  desktop-tracking-camera.service \
  desktop-tracking-rpi-stack.service
REMOTE
fi

if [[ "$START_AFTER_DEPLOY" -eq 1 ]]; then
  if [[ "$ENABLE_BOOT_SERVICES" -eq 1 ]]; then
    remote_run <<'REMOTE'
set -euo pipefail
if ! sudo -n true 2>/dev/null; then
  echo "ERROR: passwordless sudo is required to start systemd services." >&2
  exit 3
fi
sudo systemctl restart \
  desktop-tracking-camera.service \
  desktop-tracking-rpi-stack.service
REMOTE
  else
    remote_run <<'REMOTE'
set -euo pipefail
set +u
source /opt/ros/jazzy/setup.bash
source "$REMOTE_WS/install/setup.bash"
set -u
export ROBOT_DDS_ROLE=rpi
export ROBOT_WSL_IP="$WSL_TARGET_IP"
source "$REMOTE_PROJECT/scripts/ros2_network_env.sh"
nohup "$REMOTE_PROJECT/scripts/start_camera_stream.sh" "$WSL_TARGET_IP" 5600 \
  > "$REMOTE_PROJECT/logs/camera_stream.log" 2>&1 &
nohup ros2 launch robot_bringup rpi_stack.launch.py \
  > "$REMOTE_PROJECT/logs/rpi_stack.log" 2>&1 &
REMOTE
  fi
fi

echo "Deploy complete."
if [[ "$ENABLE_BOOT_SERVICES" -eq 1 ]]; then
  echo "Boot services enabled: desktop-tracking-camera.service, desktop-tracking-rpi-stack.service"
fi
