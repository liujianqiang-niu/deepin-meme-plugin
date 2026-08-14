#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
# 生成 5 个主题包的动画资源 (drawbox+fade 方案)
set -uo pipefail

THEMES_ROOT="/home/uos/work-ljq/work-v25/code-work/ai-work/deepin-meme-plugin/data/themes"

gen_wallpaper() {
    dir="$1" bg="$2" name="$3"
    ffmpeg -y -f lavfi -i "color=s=1920x1080:d=10:c=${bg}:r=30" \
        -vf "drawtext=text='${name}':fontcolor=white@0.3:fontsize=80:x=(w-text_w)/2:y=(h-text_h)/2,format=yuv420p" \
        -c:v libx264 -pix_fmt yuv420p -t 10 "$dir/wallpaper.mp4" 2>&1 | tail -1
    ffmpeg -y -i "$dir/wallpaper.mp4" -vframes 1 -q:v 2 "$dir/wallpaper.thumbnail.jpg" 2>&1 | tail -1 || true
}

gen_effect() {
    out="$1" dur="$2" vf="$3"
    ffmpeg -y -f lavfi -i "color=s=400x400:d=${dur}:c=black@0:r=30,format=rgba" \
        -vf "${vf},format=yuva420p" \
        -c:v libvpx-vp9 -pix_fmt yuva420p -auto-alt-ref 0 -t "$dur" -r 30 "$out" 2>&1 | tail -1 || true
}

gen_sound() {
    out="$1" freq="$2" dur="$3"
    ffmpeg -y -f lavfi -i "sine=frequency=${freq}:duration=${dur}" -ac 1 "$out" 2>&1 | tail -1 || true
}

mk_delete()  { echo "drawbox=x=190:y=0:w=20:h=400:color=${1}@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=1:d=$(echo "${2}-1" | bc):alpha=1"; }
mk_create()  { echo "drawbox=x=160:y=160:w=80:h=80:color=${1}@0.8:t=fill,fade=t=in:st=0:d=${2}:alpha=1"; }
mk_rename()  { echo "drawbox=x=0:y=195:w=400:h=10:color=${1}@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=0.7:d=0.3:alpha=1,fade=t=in:st=1:d=0.3:alpha=1,fade=t=out:st=1.7:d=0.3:alpha=1"; }
mk_move()    { echo "drawbox=x=100:y=180:w=40:h=40:color=${1}@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=$(echo "${2}-0.5" | bc):d=0.5:alpha=1"; }
mk_copy()    { echo "drawbox=x=140:y=180:w=30:h=40:color=${1}@0.8:t=fill,drawbox=x=230:y=180:w=30:h=40:color=${1}@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=$(echo "${2}-0.5" | bc):d=0.5:alpha=1"; }

THEMES=(
  "laser-hero|0x0a0a3e|red|blue|gold|purple|blue|Laser Hero"
  "plant-guard|0x0a2e0a|green|lime|green|lime|green|Plant Guard"
  "ninja-slash|0x1a1a1a|white|gray|silver|white|white|Ninja Slash"
  "cosmic-star|0x0a0a2e|purple|blue|purple|purple|purple|Cosmic Star"
  "firework-burst|0x0a0a1e|red|gold|orange|orange|red|Firework Burst"
)

for entry in "${THEMES[@]}"; do
  IFS='|' read -r id bg dc cc rc mc yc name <<< "$entry"
  dir="$THEMES_ROOT/$id"
  mkdir -p "$dir/effects"
  echo "========== $id ($name) =========="

  gen_wallpaper "$dir" "$bg" "$name"

  D=3; C=2; R=2; M=2.5; CP=2.5
  gen_effect "$dir/effects/delete.webm" "$D" "$(mk_delete "$dc" "$D")"
  gen_effect "$dir/effects/create.webm" "$C" "$(mk_create "$cc" "$C")"
  gen_effect "$dir/effects/rename.webm" "$R" "$(mk_rename "$rc" "$R")"
  gen_effect "$dir/effects/move.webm"   "$M" "$(mk_move "$mc" "$M")"
  gen_effect "$dir/effects/copy.webm"   "$CP" "$(mk_copy "$yc" "$CP")"

  gen_sound "$dir/effects/delete.wav" 80  0.5
  gen_sound "$dir/effects/create.wav" 880 0.3
  gen_sound "$dir/effects/rename.wav" 440 0.4
  gen_sound "$dir/effects/move.wav"   330 0.5
  gen_sound "$dir/effects/copy.wav"   660 0.4

  echo "$id done: $(du -sh "$dir" | cut -f1)"
  echo
done

echo "========== 全部完成 =========="
du -sh "$THEMES_ROOT"/* 2>/dev/null
