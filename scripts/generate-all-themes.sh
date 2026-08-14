#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
# 生成 5 个主题的特效动画 (每主题不同形状+运动)
set -uo pipefail

THEMES_ROOT="/home/uos/work-ljq/work-v25/code-work/ai-work/deepin-meme-plugin/data/themes"

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

# ── laser-hero: 竖条(激光) + 方块(光球) + 横线(扫描) ──────
echo "=== laser-hero ==="
gen_effect "$THEMES_ROOT/laser-hero/effects/delete.webm" 3 "drawbox=x=190:y=0:w=20:h=400:color=red@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=1:d=2:alpha=1"
gen_effect "$THEMES_ROOT/laser-hero/effects/create.webm" 2 "drawbox=x=150:y=150:w=100:h=100:color=blue@0.8:t=fill,fade=t=in:st=0:d=2:alpha=1"
gen_effect "$THEMES_ROOT/laser-hero/effects/rename.webm" 2 "drawbox=x=0:y=195:w=400:h=10:color=gold@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=0.7:d=0.3:alpha=1,fade=t=in:st=1:d=0.3:alpha=1,fade=t=out:st=1.7:d=0.3:alpha=1"
gen_effect "$THEMES_ROOT/laser-hero/effects/move.webm" 2.5 "drawbox=x=100:y=180:w=40:h=40:color=purple@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=2:d=0.5:alpha=1"
gen_effect "$THEMES_ROOT/laser-hero/effects/copy.webm" 2.5 "drawbox=x=140:y=180:w=30:h=40:color=blue@0.8:t=fill,drawbox=x=230:y=180:w=30:h=40:color=blue@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=2:d=0.5:alpha=1"

# ── plant-guard: 圆形(弹球) + 小方块(嫩芽) + 横线(藤蔓) ───
echo "=== plant-guard ==="
gen_effect "$THEMES_ROOT/plant-guard/effects/delete.webm" 3 "drawbox=x=170:y=170:w=60:h=60:color=green@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=1:d=2:alpha=1"
gen_effect "$THEMES_ROOT/plant-guard/effects/create.webm" 2 "drawbox=x=170:y=250:w=60:h=20:color=lime@0.8:t=fill,drawbox=x=185:y=180:w=30:h=80:color=lime@0.8:t=fill,fade=t=in:st=0:d=2:alpha=1"
gen_effect "$THEMES_ROOT/plant-guard/effects/rename.webm" 2 "drawbox=x=0:y=195:w=400:h=10:color=green@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=0.7:d=0.3:alpha=1,fade=t=in:st=1:d=0.3:alpha=1,fade=t=out:st=1.7:d=0.3:alpha=1"
gen_effect "$THEMES_ROOT/plant-guard/effects/move.webm" 2.5 "drawbox=x=170:y=170:w=60:h=60:color=lime@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=2:d=0.5:alpha=1"
gen_effect "$THEMES_ROOT/plant-guard/effects/copy.webm" 2.5 "drawbox=x=120:y=170:w=50:h=50:color=green@0.8:t=fill,drawbox=x=230:y=170:w=50:h=50:color=green@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=2:d=0.5:alpha=1"

# ── ninja-slash: 细横条(剑光) + 烟雾圆 + 闪烁竖线 ───────
echo "=== ninja-slash ==="
gen_effect "$THEMES_ROOT/ninja-slash/effects/delete.webm" 2 "drawbox=x=0:y=195:w=400:h=6:color=white@0.9:t=fill,fade=t=in:st=0:d=0.2:alpha=1,fade=t=out:st=0.5:d=1.5:alpha=1"
gen_effect "$THEMES_ROOT/ninja-slash/effects/create.webm" 2 "drawbox=x=150:y=150:w=100:h=100:color=gray@0.5:t=fill,fade=t=in:st=0:d=2:alpha=1"
gen_effect "$THEMES_ROOT/ninja-slash/effects/rename.webm" 2 "drawbox=x=195:y=0:w=10:h=400:color=silver@0.8:t=fill,fade=t=in:st=0:d=0.2:alpha=1,fade=t=out:st=0.5:d=0.3:alpha=1,fade=t=in:st=1:d=0.2:alpha=1,fade=t=out:st=1.5:d=0.5:alpha=1"
gen_effect "$THEMES_ROOT/ninja-slash/effects/move.webm" 2.5 "drawbox=x=150:y=195:w=100:h=10:color=white@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=2:d=0.5:alpha=1"
gen_effect "$THEMES_ROOT/ninja-slash/effects/copy.webm" 2.5 "drawbox=x=140:y=170:w=40:h=60:color=white@0.8:t=fill,drawbox=x=220:y=170:w=40:h=60:color=white@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=2:d=0.5:alpha=1"

# ── cosmic-star: 大圆形(星尘扩散) + 小圆(凝聚) + 闪烁 ───
echo "=== cosmic-star ==="
gen_effect "$THEMES_ROOT/cosmic-star/effects/delete.webm" 3 "drawbox=x=100:y=100:w=200:h=200:color=purple@0.6:t=fill,fade=t=in:st=0:d=0.5:alpha=1,fade=t=out:st=1:d=2:alpha=1"
gen_effect "$THEMES_ROOT/cosmic-star/effects/create.webm" 2.5 "drawbox=x=170:y=170:w=60:h=60:color=blue@0.8:t=fill,fade=t=in:st=0:d=2.5:alpha=1"
gen_effect "$THEMES_ROOT/cosmic-star/effects/rename.webm" 2 "drawbox=x=195:y=195:w=10:h=10:color=purple@0.8:t=fill,fade=t=in:st=0:d=0.2:alpha=1,fade=t=out:st=0.3:d=0.2:alpha=1,fade=t=in:st=0.6:d=0.2:alpha=1,fade=t=out:st=0.9:d=0.2:alpha=1,fade=t=in:st=1.2:d=0.2:alpha=1,fade=t=out:st=1.5:d=0.5:alpha=1"
gen_effect "$THEMES_ROOT/cosmic-star/effects/move.webm" 2.5 "drawbox=x=170:y=170:w=60:h=60:color=purple@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=2:d=0.5:alpha=1"
gen_effect "$THEMES_ROOT/cosmic-star/effects/copy.webm" 2.5 "drawbox=x=120:y=170:w=50:h=50:color=purple@0.8:t=fill,drawbox=x=230:y=170:w=50:h=50:color=purple@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=2:d=0.5:alpha=1"

# ── firework-burst: 十字(烟花爆炸) + 上升竖条 + 闪烁 ───
echo "=== firework-burst ==="
gen_effect "$THEMES_ROOT/firework-burst/effects/delete.webm" 3.5 "drawbox=x=190:y=0:w=20:h=400:color=red@0.8:t=fill,drawbox=x=0:y=190:w=400:h=20:color=red@0.8:t=fill,fade=t=in:st=0:d=0.5:alpha=1,fade=t=out:st=1.5:d=2:alpha=1"
gen_effect "$THEMES_ROOT/firework-burst/effects/create.webm" 3 "drawbox=x=190:y=300:w=20:h=100:color=gold@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=0.5:d=0.3:alpha=1,drawbox=x=170:y=170:w=60:h=60:color=gold@0.8:t=fill,fade=t=in:st=0.5:d=0.5:alpha=1,fade=t=out:st=2:d=1:alpha=1"
gen_effect "$THEMES_ROOT/firework-burst/effects/rename.webm" 2 "drawbox=x=170:y=170:w=60:h=60:color=orange@0.8:t=fill,fade=t=in:st=0:d=0.2:alpha=1,fade=t=out:st=0.3:d=0.2:alpha=1,fade=t=in:st=0.6:d:0.2:alpha=1,fade=t=out:st=0.9:d=0.2:alpha=1,fade=t=in:st=1.2:d:0.2:alpha=1,fade=t=out:st=1.5:d=0.5:alpha=1"
gen_effect "$THEMES_ROOT/firework-burst/effects/move.webm" 2.5 "drawbox=x=170:y=170:w=60:h=60:color=orange@0.8:t=fill,fade=t=in:st=0:d=0.3:alpha=1,fade=t=out:st=2:d:0.5:alpha=1"
gen_effect "$THEMES_ROOT/firework-burst/effects/copy.webm" 2.5 "drawbox=x=120:y=170:w=50:h=50:color=red@0.8:t=fill,drawbox=x=230:y=170:w=50:h=50:color=red@0.8:t=fill,fade=t=in:st=0:d:0.3:alpha=1,fade=t=out:st=2:d=0.5:alpha=1"

# ── 音效 ──────────────────────────────────────────────
for t in laser-hero plant-guard ninja-slash cosmic-star firework-burst; do
  gen_sound "$THEMES_ROOT/$t/effects/delete.wav" 80  0.5
  gen_sound "$THEMES_ROOT/$t/effects/create.wav" 880 0.3
  gen_sound "$THEMES_ROOT/$t/effects/rename.wav" 440 0.4
  gen_sound "$THEMES_ROOT/$t/effects/move.wav"   330 0.5
  gen_sound "$THEMES_ROOT/$t/effects/copy.wav"   660 0.4
done

echo "=== done ==="
for t in laser-hero plant-guard ninja-slash cosmic-star firework-burst; do
  echo "$t: $(du -sh "$THEMES_ROOT/$t" | cut -f1)"
done
