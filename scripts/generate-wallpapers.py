#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
# 生成 5 个主题的静态壁纸图片 (1920x1080)
import math, random, os
from PIL import Image, ImageDraw, ImageFont, ImageFilter

THEMES_ROOT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "data", "themes")
W, H = 1920, 1080

def radial_gradient(img, center, inner, outer, max_r):
    bg = Image.new("RGB", (W, H), outer)
    pixels = bg.load()
    cx, cy = center
    for y in range(0, H, 2):
        for x in range(0, W, 2):
            d = math.sqrt((x-cx)**2 + (y-cy)**2) / max_r
            d = min(d, 1.0)
            r = int(inner[0]*(1-d) + outer[0]*d)
            g = int(inner[1]*(1-d) + outer[1]*d)
            b = int(inner[2]*(1-d) + outer[2]*d)
            pixels[x, y] = (r, g, b)
            if x+1 < W: pixels[x+1, y] = (r, g, b)
            if y+1 < H: pixels[x, y+1] = (r, g, b)
            if x+1 < W and y+1 < H: pixels[x+1, y+1] = (r, g, b)
    return bg

def add_text(img, text, size=72, color=(255,255,255,60)):
    overlay = Image.new("RGBA", (W, H), (0,0,0,0))
    d = ImageDraw.Draw(overlay)
    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", size)
    except:
        font = ImageFont.load_default()
    bbox = d.textbbox((0,0), text, font=font)
    d.text(((W-bbox[2])//2, (H-bbox[3])//2), text, font=font, fill=color)
    return Image.alpha_composite(img.convert("RGBA"), overlay).convert("RGB")

# ── 激光英雄: 太空背景 + 激光束 ────────────────────────────
def gen_laser_hero():
    img = radial_gradient(None, (W//2, H//2), (20,20,80), (5,5,30), 800)
    d = ImageDraw.Draw(img)
    # 红蓝激光束
    for i in range(8):
        x = random.randint(100, W-100)
        top = random.randint(0, 200)
        bot = random.randint(H-300, H)
        color = (255,50,50) if i % 2 == 0 else (50,100,255)
        width = random.randint(3, 8)
        d.line([(x, top), (x, bot)], fill=color, width=width)
        d.ellipse([x-15, bot-15, x+15, bot+15], fill=color)
    # 英雄剪影 (简化人形)
    d.ellipse([W//2-40, H//2-60, W//2+40, H//2+20], fill=(30,30,60))  # 头
    d.rectangle([W//2-60, H//2+20, W//2+60, H//2+200], fill=(30,30,60))  # 身体
    img = add_text(img, "激光英雄", 80, (255,100,100,80))
    return img

# ── 植物守卫: 花园背景 + 植物叶片 ──────────────────────────
def gen_plant_guard():
    img = radial_gradient(None, (W//2, H//2), (10,60,20), (3,15,8), 800)
    d = ImageDraw.Draw(img)
    # 叶片 (多边形)
    for i in range(12):
        x = random.randint(50, W-50)
        y = random.randint(H//4, H-100)
        sz = random.randint(40, 80)
        pts = [(x, y-sz), (x+sz//2, y), (x, y+sz), (x-sz//2, y)]
        d.polygon(pts, fill=(20+random.randint(0,40), 100+random.randint(0,50), 30))
    # 绿色弹射球轨迹
    for i in range(5):
        x = random.randint(200, W-200)
        d.ellipse([x-20, 50, x+20, 90], fill=(100,255,100,200))
        d.line([(x, 90), (x, 300)], fill=(50,200,50), width=2)
    img = add_text(img, "植物守卫", 80, (100,255,100,80))
    return img

# ── 忍者剑客: 暗夜 + 剑光 + 樱花 ──────────────────────────
def gen_ninja_slash():
    img = radial_gradient(None, (W//2, H//2), (40,40,50), (10,10,15), 800)
    d = ImageDraw.Draw(img)
    # 剑光 (白色斜线)
    for i in range(5):
        x1 = random.randint(100, W//2)
        d.line([(x1, H), (x1+400, 0)], fill=(220,220,230), width=random.randint(2,5))
    # 樱花 (粉色小圆点)
    for i in range(60):
        x = random.randint(0, W)
        y = random.randint(0, H)
        r = random.randint(3, 10)
        d.ellipse([x-r, y-r, x+r, y+r], fill=(255, 180+random.randint(0,50), 200, 150))
    img = add_text(img, "忍者剑客", 80, (220,220,230,80))
    return img

# ── 宇宙星辰: 星空 + 星尘 ────────────────────────────────
def gen_cosmic_star():
    img = radial_gradient(None, (W//2, H//2), (30,20,80), (5,5,25), 800)
    d = ImageDraw.Draw(img)
    # 星辰
    for i in range(200):
        x = random.randint(0, W)
        y = random.randint(0, H)
        br = random.randint(150, 255)
        r = random.choice([1,1,1,2,2,3])
        d.ellipse([x-r, y-r, x+r, y+r], fill=(br, br, br))
    # 紫色星云
    for i in range(5):
        x = random.randint(100, W-100)
        y = random.randint(100, H-100)
        r = random.randint(80, 200)
        d.ellipse([x-r, y-r, x+r, y+r], fill=(80+random.randint(0,50), 30, 150+random.randint(0,50), 50))
    img = img.filter(ImageFilter.GaussianBlur(3))
    # 重新画亮星 (不被模糊影响)
    d2 = ImageDraw.Draw(img)
    for i in range(50):
        x = random.randint(0, W)
        y = random.randint(0, H)
        br = 255
        r = 2
        d2.ellipse([x-r, y-r, x+r, y+r], fill=(br, br, br))
    img = add_text(img, "宇宙星辰", 80, (200,180,255,80))
    return img

# ── 烟花庆典: 夜空 + 烟花 ────────────────────────────────
def gen_firework_burst():
    img = radial_gradient(None, (W//2, H//2), (20,20,60), (3,3,15), 800)
    d = ImageDraw.Draw(img)
    # 烟花 (放射状线条)
    for i in range(4):
        cx = random.randint(200, W-200)
        cy = random.randint(150, H-300)
        color = random.choice([(255,80,80), (255,200,50), (100,200,255), (255,150,200)])
        for j in range(30):
            angle = j * 12 * math.pi / 180
            length = random.randint(60, 120)
            ex = cx + int(math.cos(angle) * length)
            ey = cy + int(math.sin(angle) * length)
            d.line([(cx, cy), (ex, ey)], fill=color, width=2)
        d.ellipse([cx-5, cy-5, cx+5, cy+5], fill=color)
    # 星星
    for i in range(100):
        x = random.randint(0, W)
        y = random.randint(0, H//2)
        d.point([x, y], fill=(200,200,220))
    img = add_text(img, "烟花庆典", 80, (255,200,100,80))
    return img

# ── 生成所有壁纸 ────────────────────────────────────────
generators = {
    "laser-hero": gen_laser_hero,
    "plant-guard": gen_plant_guard,
    "ninja-slash": gen_ninja_slash,
    "cosmic-star": gen_cosmic_star,
    "firework-burst": gen_firework_burst,
}

for tid, gen in generators.items():
    print(f"Generating {tid}...", end=" ", flush=True)
    img = gen()
    path = os.path.join(THEMES_ROOT, tid, "wallpaper.jpg")
    img.save(path, "JPEG", quality=85)
    thumb = img.resize((320, 180))
    thumb_path = os.path.join(THEMES_ROOT, tid, "wallpaper.thumbnail.jpg")
    thumb.save(thumb_path, "JPEG", quality=85)
    print(f"done ({os.path.getsize(path)//1024}KB)")
