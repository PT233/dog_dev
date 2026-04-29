#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

ROS_SETUP="/opt/ros/jazzy/setup.bash"
WS_SETUP="$PROJECT_DIR/ros2_ws/install/setup.bash"
NETWORK_SETUP="$PROJECT_DIR/scripts/ros2_network_env.sh"

if [ ! -f "$ROS_SETUP" ]; then
  echo "未找到 ROS 2 Jazzy 环境: $ROS_SETUP" >&2
  exit 1
fi

if [ ! -f "$WS_SETUP" ]; then
  echo "未找到工作区环境: $WS_SETUP" >&2
  echo "请先在 $PROJECT_DIR/ros2_ws 下完成 colcon build" >&2
  exit 1
fi

if [ ! -f "$NETWORK_SETUP" ]; then
  echo "未找到网络环境脚本: $NETWORK_SETUP" >&2
  exit 1
fi

BASE_CMD="source \"$ROS_SETUP\" && cd \"$PROJECT_DIR\" && source \"$WS_SETUP\" && export ROBOT_DDS_ROLE=wsl && source \"$NETWORK_SETUP\""

VISION_CMD="$BASE_CMD && ros2 launch robot_bringup vision_stack.launch.py; exec bash"
VIZ_CMD="$BASE_CMD && ros2 run detection_node detection_viz_node_exe; exec bash"
VIEW_CMD="$BASE_CMD && ros2 run rqt_image_view rqt_image_view /camera/image_detected; exec bash"

open_with_wt() {
  local -a wsl_cmd
  wsl_cmd=(wsl.exe)
  if [ -n "${WSL_DISTRO_NAME:-}" ]; then
    wsl_cmd+=(-d "$WSL_DISTRO_NAME")
  fi

  wt.exe -w 0 \
    new-tab --title "vision_stack" "${wsl_cmd[@]}" bash -lc "$VISION_CMD" \
    ';' \
    new-tab --title "detection_viz" "${wsl_cmd[@]}" bash -lc "$VIZ_CMD" \
    ';' \
    new-tab --title "image_detected" "${wsl_cmd[@]}" bash -lc "$VIEW_CMD"
}

open_with_linux_term() {
  local term_bin="$1"

  case "$term_bin" in
    gnome-terminal)
      gnome-terminal -- bash -lc "$VISION_CMD" &
      gnome-terminal -- bash -lc "$VIZ_CMD" &
      gnome-terminal -- bash -lc "$VIEW_CMD" &
      ;;
    konsole)
      konsole -e bash -lc "$VISION_CMD" &
      konsole -e bash -lc "$VIZ_CMD" &
      konsole -e bash -lc "$VIEW_CMD" &
      ;;
    *)
      x-terminal-emulator -e bash -lc "$VISION_CMD" &
      x-terminal-emulator -e bash -lc "$VIZ_CMD" &
      x-terminal-emulator -e bash -lc "$VIEW_CMD" &
      ;;
  esac
}

if command -v wt.exe >/dev/null 2>&1; then
  echo "使用 Windows Terminal 打开 3 个标签页..."
  open_with_wt
  exit 0
fi

if command -v gnome-terminal >/dev/null 2>&1; then
  echo "使用 gnome-terminal 打开 3 个终端..."
  open_with_linux_term gnome-terminal
  exit 0
fi

if command -v konsole >/dev/null 2>&1; then
  echo "使用 konsole 打开 3 个终端..."
  open_with_linux_term konsole
  exit 0
fi

if command -v x-terminal-emulator >/dev/null 2>&1; then
  echo "使用 x-terminal-emulator 打开 3 个终端..."
  open_with_linux_term x-terminal-emulator
  exit 0
fi

echo "没有找到可用的终端程序。" >&2
echo "请手动运行以下三条命令：" >&2
echo "1) $VISION_CMD" >&2
echo "2) $VIZ_CMD" >&2
echo "3) $VIEW_CMD" >&2
exit 1
