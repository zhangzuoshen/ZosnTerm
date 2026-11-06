#!/usr/bin/env bash
# 下载并 vendoring PuTTY 0.85 核心源码到本目录（MIT 许可，锁定版本）。
# 用法: bash vendor_putty.sh
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"

VER="0.85"
URL="https://the.earth.li/~sgtatham/putty/${VER}/putty-${VER}.tar.gz"
ARCHIVE="putty-${VER}.tar.gz"

if [ -d "putty-${VER}" ]; then
    echo "已存在 putty-${VER}/，跳过下载。如需重新拉取请先删除该目录。"
    exit 0
fi

echo "==> 下载 PuTTY ${VER}: ${URL}"
if command -v curl >/dev/null 2>&1; then
    curl -fSL "$URL" -o "$ARCHIVE"
elif command -v wget >/dev/null 2>&1; then
    wget -O "$ARCHIVE" "$URL"
else
    echo "错误：未找到 curl 或 wget，无法下载。" >&2
    exit 1
fi

echo "==> 解包到 putty-${VER}/"
mkdir -p "putty-${VER}"
tar -xzf "$ARCHIVE" -C "putty-${VER}" --strip-components=1
rm -f "$ARCHIVE"

echo "==> 完成。源码位于: $HERE/putty-${VER}"
echo "    M2 将从中挑选核心文件（serial.c/terminal.c/ldisc.c/...）编译为 libputty。"
