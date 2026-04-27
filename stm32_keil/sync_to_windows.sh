#!/bin/bash
# 将 stm32_keil 目录同步到 Windows 盘

SRC="$(cd "$(dirname "$0")" && pwd)"
DST="/mnt/h/stm32_keil"

mkdir -p "$DST"
cp -rv "$SRC/." "$DST/"

echo "同步完成: $SRC -> $DST"
