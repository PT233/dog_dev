#!/usr/bin/env bash

# Shared ROS 2 network setup for WSL2 <-> Raspberry Pi.
#
# Source this file before starting ROS 2 commands:
#   ROBOT_DDS_ROLE=wsl source scripts/ros2_network_env.sh
#   ROBOT_DDS_ROLE=rpi ROBOT_WSL_IP=192.168.137.1 source scripts/ros2_network_env.sh
#
# It forces CycloneDDS onto one interface and replaces multicast discovery with
# an explicit unicast peer list.

if [ -z "${BASH_VERSION:-}" ]; then
  echo "ros2_network_env.sh must be sourced from bash" >&2
  return 2 2>/dev/null || exit 2
fi

robot_dds_route_field() {
  local target="$1"
  local field="$2"
  ip route get "$target" 2>/dev/null | awk -v field="$field" '
    {
      for (i = 1; i <= NF; i++) {
        if ($i == field) {
          print $(i + 1)
          exit
        }
      }
    }'
}

robot_dds_write_cyclonedds_xml() {
  local config_file="$1"
  local interface_name="$2"
  local peers="$3"
  local peer

  mkdir -p "$(dirname "$config_file")"

  {
    printf '%s\n' '<?xml version="1.0" encoding="UTF-8"?>'
    printf '%s\n' '<CycloneDDS xmlns="https://cdds.io/config" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="https://cdds.io/config https://raw.githubusercontent.com/eclipse-cyclonedds/cyclonedds/master/etc/cyclonedds.xsd">'
    printf '%s\n' '  <Domain Id="any">'
    printf '%s\n' '    <General>'
    printf '      <Interfaces><NetworkInterface name="%s" multicast="false"/></Interfaces>\n' "$interface_name"
    printf '%s\n' '      <AllowMulticast>false</AllowMulticast>'
    printf '%s\n' '    </General>'
    printf '%s\n' '    <Discovery>'
    printf '%s\n' '      <ParticipantIndex>auto</ParticipantIndex>'
    printf '%s\n' '      <Peers>'
    for peer in $peers; do
      peer="${peer%,}"
      [ -n "$peer" ] || continue
      printf '        <Peer Address="%s"/>\n' "$peer"
    done
    printf '%s\n' '      </Peers>'
    printf '%s\n' '    </Discovery>'
    printf '%s\n' '  </Domain>'
    printf '%s\n' '</CycloneDDS>'
  } > "$config_file"
}

export ROS_DOMAIN_ID="${ROS_DOMAIN_ID:-42}"
export ROS_LOCALHOST_ONLY=0
export RMW_IMPLEMENTATION="${RMW_IMPLEMENTATION:-rmw_cyclonedds_cpp}"

ROBOT_DDS_ROLE="${ROBOT_DDS_ROLE:-wsl}"
ROBOT_PI_IP="${ROBOT_PI_IP:-192.168.137.100}"

case "$ROBOT_DDS_ROLE" in
  wsl)
    ROBOT_DDS_INTERFACE="${ROBOT_DDS_INTERFACE:-$(robot_dds_route_field "$ROBOT_PI_IP" dev)}"
    ROBOT_DDS_INTERFACE="${ROBOT_DDS_INTERFACE:-eth0}"
    ROBOT_DDS_PEERS="${ROBOT_DDS_PEERS:-$ROBOT_PI_IP}"
    ;;
  rpi)
    ROBOT_DDS_INTERFACE="${ROBOT_DDS_INTERFACE:-eth0}"
    ROBOT_WSL_IP="${ROBOT_WSL_IP:-${WSL_TARGET_IP:-192.168.137.1}}"
    ROBOT_DDS_PEERS="${ROBOT_DDS_PEERS:-$ROBOT_WSL_IP}"
    ;;
  *)
    echo "Unsupported ROBOT_DDS_ROLE=$ROBOT_DDS_ROLE (expected wsl or rpi)" >&2
    return 2 2>/dev/null || exit 2
    ;;
esac

ROBOT_CYCLONEDDS_CONFIG="${ROBOT_CYCLONEDDS_CONFIG:-/tmp/desktop_tracking_robot_cyclonedds_${ROBOT_DDS_ROLE}.xml}"
robot_dds_write_cyclonedds_xml "$ROBOT_CYCLONEDDS_CONFIG" "$ROBOT_DDS_INTERFACE" "$ROBOT_DDS_PEERS"
export CYCLONEDDS_URI="file://$ROBOT_CYCLONEDDS_CONFIG"

if [ "${ROBOT_DDS_QUIET:-0}" != "1" ]; then
  echo "ROS_DOMAIN_ID=$ROS_DOMAIN_ID"
  echo "RMW_IMPLEMENTATION=$RMW_IMPLEMENTATION"
  echo "CYCLONEDDS_URI=$CYCLONEDDS_URI"
  echo "CycloneDDS interface=$ROBOT_DDS_INTERFACE peers=$ROBOT_DDS_PEERS"
fi
