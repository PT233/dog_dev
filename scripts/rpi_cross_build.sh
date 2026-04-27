#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RPI_SYSROOT="${RPI_SYSROOT:-$HOME/rpi-sysroot}"
INSTALL_BASE="${INSTALL_BASE:-$ROOT_DIR/ros2_ws/install_rpi_aarch64}"
BUILD_BASE="${BUILD_BASE:-$ROOT_DIR/ros2_ws/build_rpi_aarch64}"
LOG_BASE="${LOG_BASE:-$ROOT_DIR/ros2_ws/log_rpi_aarch64}"
TARBALL="${TARBALL:-$ROOT_DIR/deploy/ros2_ws_install_rpi_aarch64.tgz}"
PACKAGES=(robot_interfaces uart_bridge robot_bringup)

usage() {
  cat <<'EOF'
Usage:
  scripts/rpi_cross_build.sh

Builds the Raspberry Pi ROS package subset for ARM64 from WSL using an ARM64
sysroot. This script needs a sysroot copied from a Pi that already has ROS 2
Jazzy installed.

Required:
  RPI_SYSROOT=/path/to/rpi-sysroot

Typical sysroot layout:
  $RPI_SYSROOT/opt/ros/jazzy
  $RPI_SYSROOT/usr/include
  $RPI_SYSROOT/usr/lib/aarch64-linux-gnu

Outputs:
  ros2_ws/install_rpi_aarch64
  deploy/ros2_ws_install_rpi_aarch64.tgz
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

command -v aarch64-linux-gnu-gcc >/dev/null 2>&1 || {
  echo "ERROR: missing aarch64-linux-gnu-gcc" >&2
  exit 1
}
command -v aarch64-linux-gnu-g++ >/dev/null 2>&1 || {
  echo "ERROR: missing aarch64-linux-gnu-g++" >&2
  exit 1
}
command -v colcon >/dev/null 2>&1 || {
  echo "ERROR: missing colcon" >&2
  exit 1
}

[[ -d "$RPI_SYSROOT/opt/ros/jazzy" ]] || {
  echo "ERROR: missing $RPI_SYSROOT/opt/ros/jazzy" >&2
  echo "Create it after the Pi is reachable, for example:" >&2
  echo "  mkdir -p \"$RPI_SYSROOT/opt/ros\" \"$RPI_SYSROOT/usr\"" >&2
  echo "  rsync -a ubuntu@192.168.137.100:/opt/ros/jazzy \"$RPI_SYSROOT/opt/ros/\"" >&2
  echo "  rsync -a ubuntu@192.168.137.100:/usr/include \"$RPI_SYSROOT/usr/\"" >&2
  echo "  rsync -a ubuntu@192.168.137.100:/usr/lib/aarch64-linux-gnu \"$RPI_SYSROOT/usr/lib/\"" >&2
  exit 2
}

[[ -d "$RPI_SYSROOT/usr/lib/aarch64-linux-gnu" ]] || {
  echo "ERROR: missing $RPI_SYSROOT/usr/lib/aarch64-linux-gnu" >&2
  exit 2
}

mkdir -p "$(dirname "$TARBALL")"

source /opt/ros/jazzy/setup.bash
export RPI_SYSROOT

cd "$ROOT_DIR/ros2_ws"
colcon build \
  --merge-install \
  --build-base "$BUILD_BASE" \
  --install-base "$INSTALL_BASE" \
  --log-base "$LOG_BASE" \
  --packages-select "${PACKAGES[@]}" \
  --allow-overriding robot_interfaces \
  --cmake-args \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/cmake/toolchains/rpi4-aarch64-ros2.cmake" \
    -DBUILD_TESTING=OFF

tar -C "$INSTALL_BASE" -czf "$TARBALL" .
echo "Wrote $TARBALL"
