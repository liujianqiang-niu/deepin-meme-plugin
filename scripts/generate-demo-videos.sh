#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
# 生成 deepin-meme-plugin 占位演示视频资源
set -euo pipefail

THEME_DIR="/home/uos/work-ljq/work-v25/code-work/ai-work/deepin-meme-plugin/data/themes/example"
EFFECTS_DIR="$THEME_DIR/effects"
mkdir -p "$EFFECTS_DIR"

echo "=== 1/6 wallpaper.mp4 ==="
ffmpeg -y -f lavfi -i "testsrc2=s=1920x1080:d=10:r=30" \
    -vf "drawtext=text='Deepin Meme Demo':fontcolor=white@0.5:fontsize=72:x=(w-text_w)/2:y=(h-text_h)/2,format=yuv420p" \
    -c:v libx264 -pix_fmt yuv420p -t 10 "$THEME_DIR/wallpaper.mp4" 2>&1 | tail -2

ffmpeg -y -i "$THEME_DIR/wallpaper.mp4" -vframes 1 -q:v 2 "$THEME_DIR/wallpaper.thumbnail.jpg" 2>&1 | tail -1

gen_effect() {
    local out="$1" dur="$2" color="$3" name="$4"
    ffmpeg -y -f lavfi -i "color=s=400x400:d=${dur}:c=black@0,format=rgba" \
        -vf "drawtext=text='${name}':fontcolor=${color}:fontsize=64:x=(w-text_w)/2:y=(h-text_h)/2:alpha='if(lt(t,0.3),t/0.3,if(gt(t,${dur}-0.5),(${dur}-t)/0.5,1))',format=rgba" \
        -c:v libvpx-vp9 -pix_fmt yuva420p -auto-alt-ref 0 -t "${dur}" -r 30 "$out" 2>&1 | tail -2
}

echo "=== 2/6 delete.webm ==="
gen_effect "$EFFECTS_DIR/delete.webm" 3 "red" "DELETE"
echo "=== 3/6 create.webm ==="
gen_effect "$EFFECTS_DIR/create.webm" 2 "green" "CREATE"
echo "=== 4/6 rename.webm ==="
gen_effect "$EFFECTS_DIR/rename.webm" 2 "blue" "RENAME"
echo "=== 5/6 move.webm ==="
gen_effect "$EFFECTS_DIR/move.webm" 2.5 "yellow" "MOVE"
echo "=== 6/6 copy.webm ==="
gen_effect "$EFFECTS_DIR/copy.webm" 2.5 "magenta" "COPY"

echo "=== 音效 ==="
ffmpeg -y -f lavfi -i "sine=frequency=80:duration=0.5" "$EFFECTS_DIR/delete.wav" 2>&1 | tail -1
ffmpeg -y -f lavfi -i "sine=frequency=880:duration=0.3" "$EFFECTS_DIR/create.wav" 2>&1 | tail -1
ffmpeg -y -f lavfi -i "sine=frequency=440:duration=0.4" "$EFFECTS_DIR/rename.wav" 2>&1 | tail -1
ffmpeg -y -f lavfi -i "sine=frequency=330:duration=0.5" "$EFFECTS_DIR/move.wav" 2>&1 | tail -1
ffmpeg -y -f lavfi -i "sine=frequency=660:duration=0.4" "$EFFECTS_DIR/copy.wav" 2>&1 | tail -1

echo "=== 产物列表 ==="
ls -lhR "$THEME_DIR"
