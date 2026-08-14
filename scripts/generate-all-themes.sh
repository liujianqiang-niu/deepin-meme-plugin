#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
# 为 5 个主题包批量生成差异化占位演示视频
# 每个主题有不同的壁纸颜色 + 不同的特效风格
set -euo pipefail

THEMES_ROOT="/home/uos/work-ljq/work-v25/code-work/ai-work/deepin-meme-plugin/data/themes"

# 主题配置: id|壁纸色相|delete色|create色|rename色|move色|copy色|effect风格
THEMES=(
  "scifi-hero|240|red|cyan|blue|yellow|magenta|laser"
  "pixel-monster|120|lime|yellow|orange|cyan|magenta|pixel"
  "cosmic|270|purple|white|blue|magenta|cyan|sparkle"
  "fireworks|30|orange|gold|red|yellow|green|burst"
  "ninja|0|white|red|silver|darkred|black|slash"
)

# 生成壁纸(10秒循环,带主题色)
gen_wallpaper() {
    local dir="$1" hue="$2" name="$3"
    ffmpeg -y -f lavfi -i "testsrc2=s=1920x1080:d=10:r=30" \
        -vf "hue=h=${hue}:s=1.5,drawtext=text='${name}':fontcolor=white@0.6:fontsize=72:x=(w-text_w)/2:y=(h-text_h)/2,format=yuv420p" \
        -c:v libx264 -pix_fmt yuv420p -t 10 "$dir/wallpaper.mp4" 2>&1 | tail -1
    ffmpeg -y -i "$dir/wallpaper.mp4" -vframes 1 -q:v 2 "$dir/wallpaper.thumbnail.jpg" 2>&1 | tail -1
}

# 生成特效视频(带 alpha)
gen_effect() {
    local out="$1" dur="$2" color="$3" label="$4"
    ffmpeg -y -f lavfi -i "color=s=400x400:d=${dur}:c=black@0,format=rgba" \
        -vf "drawtext=text='${label}':fontcolor=${color}:fontsize=56:x=(w-text_w)/2:y=(h-text_h)/2:alpha='if(lt(t,0.3),t/0.3,if(gt(t,${dur}-0.5),(${dur}-t)/0.5,1))',format=rgba" \
        -c:v libvpx-vp9 -pix_fmt yuva420p -auto-alt-ref 0 -t "${dur}" -r 30 "$out" 2>&1 | tail -1
}

# 生成音效
gen_sound() {
    local out="$1" freq="$2" dur="$3"
    ffmpeg -y -f lavfi -i "sine=frequency=${freq}:duration=${dur}" "$out" 2>&1 | tail -1
}

for theme_entry in "${THEMES[@]}"; do
    IFS='|' read -r id hue dcol ccol rcol mcol cpycol style <<< "$theme_entry"
    dir="$THEMES_ROOT/$id"
    echo "========== 生成主题: $id ($style 风格) =========="

    # 壁纸
    case "$id" in
        scifi-hero) wname="Sci-Fi Hero" ;;
        pixel-monster) wname="Pixel Monster" ;;
        cosmic) wname="Cosmic" ;;
        fireworks) wname="Fireworks" ;;
        ninja) wname="Ninja" ;;
    esac
    gen_wallpaper "$dir" "$hue" "$wname"

    # 5 个特效(每个主题特效时长不同)
    case "$style" in
        laser)   ddur=3; cdur=2; rdur=2; mdur=2.5; cpdur=2.5 ;;
        pixel)   ddur=2.5; cdur=2; rdur=2; mdur=2.5; cpdur=2.5 ;;
        sparkle)  ddur=3; cdur=2.5; rdur=2; mdur=2.5; cpdur=2.5 ;;
        burst)   ddur=3.5; cdur=3; rdur=2; mdur=2.5; cpdur=2.5 ;;
        slash)   ddur=2; cdur=2; rdur=2; mdur=2.5; cpdur=2.5 ;;
    esac

    gen_effect "$dir/effects/delete.webm" "$ddur" "$dcol" "DESTROY"
    gen_effect "$dir/effects/create.webm" "$cdur" "$ccol" "BIRTH"
    gen_effect "$dir/effects/rename.webm" "$rdur" "$rcol" "MORPH"
    gen_effect "$dir/effects/move.webm" "$mdur" "$mcol" "SHIFT"
    gen_effect "$dir/effects/copy.webm" "$cpdur" "$cpycol" "CLONE"

    # 音效(每个主题不同频率)
    case "$style" in
        laser)   df=80; cf=880; rf=440; mf=330; cpf=660 ;;
        pixel)   df=120; cf=660; rf=520; mf=400; cpf=500 ;;
        sparkle) df=200; cf=1000; rf=600; mf=450; cpf=700 ;;
        burst)   df=100; cf=900; rf=550; mf=380; cpf=620 ;;
        slash)   df=150; cf=770; rf=480; mf=350; cpf=580 ;;
    esac
    gen_sound "$dir/effects/delete.wav" "$df" 0.5
    gen_sound "$dir/effects/create.wav" "$cf" 0.3
    gen_sound "$dir/effects/rename.wav" "$rf" 0.4
    gen_sound "$dir/effects/move.wav" "$mf" 0.5
    gen_sound "$dir/effects/copy.wav" "$cpf" 0.4

    echo "主题 $id 完成: $(du -sh "$dir" | cut -f1)"
    echo ""
done

echo "========== 全部主题生成完成 =========="
echo "总览:"
du -sh "$THEMES_ROOT"/*
