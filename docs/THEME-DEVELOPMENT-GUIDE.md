# Deepin Meme Plugin 主题开发指南

本指南教你如何新增动画和动效——创建自定义主题包，或扩展现有主题。

---

## 1. 架构概览

### 1.1 主题包是什么

每个"主题包"是一个目录，包含动态壁纸视频 + 5 种操作特效视频 + 音效 + manifest.json 清单。插件加载时扫描主题目录，解析 manifest，按用户选择应用对应主题。

### 1.2 当前内置 6 个主题

| 主题 ID | 名称 | 风格 |
|---------|------|------|
| `example` | 示例 | 基础占位 |
| `scifi-hero` | 科幻英雄 | 激光摧毁 |
| `pixel-monster` | 像素怪兽 | 8-bit 复古 |
| `cosmic` | 宇宙星辰 | 星空消散 |
| `fireworks` | 烟花庆典 | 烟花绽放 |
| `ninja` | 忍者剑客 | 剑击消散 |

### 1.3 主题包搜索路径

插件按以下顺序扫描主题包（先找到的优先）：

1. `~/.local/share/deepin-meme-themes/` — 用户级（无需 root）
2. `/usr/share/deepin-meme-themes/` — 系统级（deb 包安装）

---

## 2. 主题包目录结构

```
my-theme/
├── manifest.json              ← 必须,清单文件
├── wallpaper.mp4              ← 必须,循环播放的动态壁纸
├── wallpaper.thumbnail.jpg    ← 推荐,缩略图(控制中心预览用)
└── effects/                   ← 特效视频目录
    ├── delete.webm             ← 删除特效视频(带 alpha 通道)
    ├── delete.wav              ← 删除音效
    ├── create.webm             ← 新建特效
    ├── create.wav
    ├── rename.webm             ← 重命名特效
    ├── rename.wav
    ├── move.webm               ← 移动特效
    ├── move.wav
    ├── copy.webm               ← 复制特效
    └── copy.wav
```

---

## 3. manifest.json 字段说明

```json
{
  "id": "my-theme",              // 主题唯一标识(小写字母数字横线)
  "name": "我的主题",             // 显示名称
  "description": "主题描述",      // 可选
  "version": "1.0.0",            // 版本号
  "author": "你的名字",           // 可选
  "wallpaper": "wallpaper.mp4",  // 壁纸视频相对路径
  "thumbnail": "wallpaper.thumbnail.jpg",  // 缩略图相对路径
  "effects": {
    "delete": {                  // 操作类型: delete/create/rename/move/copy
      "video": "effects/delete.webm",   // 特效视频(推荐 webm 带 alpha)
      "audio": "effects/delete.wav",    // 音效(可选)
      "anchor": "target",               // 锚点: target(文件坐标) | screen-center
      "scale": 1.5,                     // 缩放系数(1.0=原始大小)
      "duration": 3000                  // 持续时长(毫秒)
    },
    "create": { "video": "...", "audio": "...", "anchor": "target", "scale": 1.0, "duration": 2000 },
    "rename": { "video": "...", "audio": "...", "anchor": "target", "scale": 1.0, "duration": 2000 },
    "move":   { "video": "...", "audio": "...", "anchor": "target", "scale": 1.0, "duration": 2500 },
    "copy":   { "video": "...", "audio": "...", "anchor": "target", "scale": 1.0, "duration": 2500 }
  }
}
```

### 3.1 必填字段

- `id` — 唯一标识，用于切换主题
- `name` — 显示名称
- `version` — 版本号
- `wallpaper` — 壁纸视频路径
- `effects` — 至少包含 `delete` 一个特效

### 3.2 可选特效

5 种操作特效都是**可选**的。如果某操作没有配置特效，该操作不会触发动画。你可以只配置 `delete` 和 `create`，其余操作静默。

### 3.3 锚点(anchor)说明

| 值 | 效果 |
|----|------|
| `target` | 特效中心对准被操作文件的桌面坐标(推荐) |
| `screen-center` | 特效居中于屏幕 |

---

## 4. 视频格式要求

### 4.1 推荐格式

| 文件类型 | 推荐格式 | 说明 |
|----------|----------|------|
| 壁纸 video | `mp4` (H.264) | 通用性最好，Qt6 Multimedia 全平台支持 |
| 特效 video | `webm` (VP9 + yuva420p) | **带 alpha 通道**，实现透明叠加 |
| 音效 | `wav` 或 `ogg` | 无损优先 |

### 4.2 为什么特效视频需要 alpha 通道

特效叠加在桌面上播放，背景必须透明才能看到桌面。`webm` 格式支持 `yuva420p` 像素格式（含 alpha 通道），实现透明背景。

### 4.3 格式转换命令

**将绿幕视频转为带 alpha 的 webm**（最常见场景，绿幕素材去背景）：

```bash
# 假设你有一个绿幕 mp4，背景是纯绿色
ffmpeg -i green-screen.mp4 \
    -vf "chromakey=0x00ff00:0.3:0.1,format=yuva420p" \
    -c:v libvpx-vp9 \
    -auto-alt-ref 0 \
    -b:v 1M \
    output.webm
```

参数说明：
- `chromakey=0x00ff00:0.3:0.1` — 抠掉绿色(0x00ff00)，相似度阈值 0.3，混合度 0.1
- `format=yuva420p` — 输出带 alpha 通道的像素格式
- `auto-alt-ref 0` — 禁用 VP9 的替代参考帧(否则 alpha 丢失)

**将普通 mp4 转为 webm**（无 alpha，不透明叠加）：

```bash
ffmpeg -i input.mp4 -c:v libvpx-vp9 -b:v 1M output.webm
```

**调整视频分辨率**：

```bash
# 缩放到 400x400(特效推荐尺寸)
ffmpeg -i input.mp4 -vf scale=400:400 output.webm
```

**裁剪视频时长**：

```bash
# 截取前 3 秒
ffmpeg -i input.mp4 -t 3 output.webm
```

---

## 5. 如何新增一个主题包（分步教程

### 5.1 创建目录

```bash
mkdir -p ~/.local/share/deepin-meme-themes/my-theme/effects
cd ~/.local/share/deepin-meme-themes/my-theme
```

> 放到 `~/.local/share/` 下无需 root 权限，即时生效（重启守护进程或调用 `ReloadThemes` D-Bus 方法）。

### 5.2 准备视频资源

你需要：
- 1 个壁纸视频（mp4，1920x1080，10-30 秒循环）
- 5 个特效视频（webm，400x400，带 alpha 通道）
- 5 个音效文件（wav，0.3-0.5 秒）

资源获取途径见 `docs/superpowers/specs/2026-08-14-deepin-meme-plugin-design.md` 第 11 节。

### 5.3 编写 manifest.json

```bash
cat > manifest.json << 'EOF'
{
  "id": "my-theme",
  "name": "我的主题",
  "version": "1.0.0",
  "wallpaper": "wallpaper.mp4",
  "thumbnail": "wallpaper.thumbnail.jpg",
  "effects": {
    "delete": {
      "video": "effects/delete.webm",
      "audio": "effects/delete.wav",
      "anchor": "target",
      "scale": 1.0,
      "duration": 3000
    }
  }
}
EOF
```

> 上面只配置了 `delete`，其他操作不会有特效。按需添加。

### 5.4 生成缩略图

```bash
ffmpeg -i wallpaper.mp4 -vframes 1 -q:v 2 wallpaper.thumbnail.jpg
```

### 5.5 重载主题

```bash
# 方法 1: 重启守护进程
systemctl --user restart deepin-meme-daemon

# 方法 2: 通过 D-Bus 热重载(无需重启)
dbus-send --session --print-reply \
    --dest=org.deepin.meme.daemon \
    /org/deepin/meme/daemon \
    org.deepin.meme.daemon.ReloadThemes
```

### 5.6 验证

打开控制中心 → 个性化 → 趣味壁纸 → 主题下拉框应出现你的主题。

---

## 6. 扩展操作类型（高级）

### 6.1 当前支持的 5 种操作

| 操作类型 | 触发条件 |
|----------|----------|
| `delete` | 桌面文件被删除或移到回收站 |
| `create` | 桌面新建文件或目录 |
| `rename` | 桌面文件重命名 |
| `move` | 文件移动到桌面 |
| `copy` | 文件复制到桌面 |

### 6.2 如何新增一种操作类型

**场景**: 你想在用户"双击打开文件"时触发特效。

#### 步骤 1: 扩展 EffectType 枚举

`src/common/src/thememanifest.h`:
```cpp
enum class EffectType {
    Delete,
    Create,
    Rename,
    Move,
    Copy,
    Open   // ← 新增
};
```

#### 步骤 2: 更新字符串映射

`src/common/src/thememanifest.cpp`:
```cpp
QString effectTypeToString(EffectType type) {
    switch (type) {
    case EffectType::Delete: return "delete";
    case EffectType::Create: return "create";
    case EffectType::Rename: return "rename";
    case EffectType::Move:   return "move";
    case EffectType::Copy:   return "copy";
    case EffectType::Open:   return "open";  // ← 新增
    }
    return {};
}

std::optional<EffectType> stringToEffectType(const QString &str) {
    if (str == "delete") return EffectType::Delete;
    if (str == "create") return EffectType::Create;
    if (str == "rename") return EffectType::Rename;
    if (str == "move")   return EffectType::Move;
    if (str == "copy")   return EffectType::Copy;
    if (str == "open")   return EffectType::Open;  // ← 新增
    return std::nullopt;
}
```

#### 步骤 3: 在 FileOperationMonitor 中监听新事件

`src/desktop-daemon/src/fileoperationmonitor.cpp` 的 `onDirectoryChanged` 中，添加对新操作的检测逻辑，然后：
```cpp
emit fileOperationDetected(QStringLiteral("open"), filePath);
```

#### 步骤 4: 在主题包 manifest.json 中配置新特效

```json
{
  "effects": {
    "delete": { "video": "effects/delete.webm", ... },
    "open": {
      "video": "effects/open.webm",
      "audio": "effects/open.wav",
      "anchor": "target",
      "scale": 1.0,
      "duration": 1500
    }
  }
}
```

#### 步骤 5: 重新编译

```bash
cd build && cmake --build . -j$(nproc)
```

manifest.json 中的 `effects` 对象支持任意键名（`additionalProperties` 在 schema 中允许），所以主题包无需改代码即可使用新操作类型——只要守护进程能发射对应事件字符串。

---

## 7. 主题包分发

### 7.1 用户级安装（无需 root）

```bash
# 复制主题包到用户目录
cp -r my-theme ~/.local/share/deepin-meme-themes/

# 重载
dbus-send --session --print-reply --dest=org.deepin.meme.daemon \
    /org/deepin/meme/daemon org.deepin.meme.daemon.ReloadThemes
```

### 7.2 系统级安装（打 deb 包）

将主题包放入 `data/themes/` 目录，在 CMakeLists.txt 的 `install(DIRECTORY ...)` 中添加主题名，重新构建 deb 包。

### 7.3 主题包打包为独立 deb

```bash
# 创建独立主题包 deb
mkdir -p my-theme-deb/usr/share/deepin-meme-themes/my-theme
cp -r my-theme/* my-theme-deb/usr/share/deepin-meme-themes/my-theme/
# 编写 debian/control, rules 等
cd my-theme-deb && dpkg-buildpackage -us -uc -b
```

---

## 8. 调试技巧

### 8.1 查看守护进程日志

```bash
# 实时日志
journalctl --user -u deepin-meme-daemon -f

# 或直接运行(前台模式)
/usr/bin/deepin-meme-daemon
```

### 8.2 通过 D-Bus 手动触发特效

```bash
# 预览删除特效
dbus-send --session --print-reply --dest=org.deepin.meme.daemon \
    /org/deepin/meme/daemon org.deepin.meme.daemon.PreviewEffect string:"delete"

# 查看可用主题
dbus-send --session --print-reply --dest=org.deepin.meme.daemon \
    /org/deepin/meme/daemon org.deepin.meme.daemon.GetThemes

# 切换主题
dbus-send --session --print-reply --dest=org.deepin.meme.daemon \
    /org/deepin/meme/daemon org.deepin.meme.daemon.SetTheme string:"cosmic"
```

### 8.3 检查主题包是否被识别

```bash
dbus-send --session --print-reply --dest=org.deepin.meme.daemon \
    /org/deepin/meme/daemon org.deepin.meme.daemon.GetThemes
```

如果你的主题 ID 出现在返回列表中，说明 manifest.json 被正确解析。

---

## 9. 最佳实践

### 9.1 视频尺寸

- 壁纸: 1920x1080（或匹配屏幕分辨率）
- 特效: 400x400（正方形，便于在文件坐标处居中）
- 缩略图: 320x180

### 9.2 时长

- 壁纸: 10-30 秒（循环播放，太短会闪烁）
- 特效: 1-3 秒（太长影响操作体验）
- 音效: 0.3-0.5 秒

### 9.3 文件大小

- 壁纸: < 15MB（H.264 编码）
- 特效: < 500KB（VP9 编码）
- 总主题包: < 20MB

### 9.4 alpha 通道

特效视频**必须**带 alpha 通道才能透明叠加。用 `webm + yuva420p` 格式。如果只有绿幕素材，用 `chromakey` 滤镜去背景（见第 4.3 节）。
