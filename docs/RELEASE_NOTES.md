# deepin-meme-plugin

> 项目：deepin-meme-plugin —— Deepin / UOS v25 动态视频壁纸插件
> 版本：v3.0.0（首次发布）
> 许可：GPL-3.0-or-later
> 维护：liujianqiang@uniontech.com

---

## 项目简介

deepin-meme-plugin 是面向 Deepin / UOS v25 桌面的动态视频壁纸插件，由「桌面边缘插件」与「控制中心插件」两部分组成，让桌面壁纸动起来的同时，提供友好的管理界面与强大的视频兼容能力。

## 主要特性

### 🎬 视频壁纸渲染

- **FFmpeg 高性能解码**：支持 H264 / H265 / AV1 / VP9 等主流视频编码。
- **多级硬件加速**：软解 + VAAPI + CUDA 三级解码模式，`auto` 模式下自动探测最优硬件路径。
- **桌面嵌入渲染**：视频帧直接嵌入桌面 root 窗口，与桌面融为一体，无独立窗口干扰。
- **多屏支持**：多显示器场景下共享同一渲染资源，保持画面同步。
- **流畅帧率控制**：默认跟随视频源帧率，支持自定义帧率上限（1 ~ 240 FPS），解码超负荷时自动丢帧保实时。

### 🖼️ 显示模式

支持 5 种铺屏方式，适配不同分辨率与画幅：

| 模式 | 说明 |
|---|---|
| 铺满（fill） | 等比放大裁切，无黑边（默认） |
| 自适应（fit） | 完整显示，可能带黑边 |
| 拉伸（stretch） | 拉满屏幕，可能变形 |
| 居中（center） | 原始像素居中，不缩放 |
| 平铺（tile） | 重复铺满屏幕 |

### ⚙️ 控制中心管理

「控制中心 → 个性化 → 动态壁纸」提供一站式管理：

- **一键启用**：开关控制动态壁纸的启用与停用。
- **视频上传**：支持 mp4 / mkv / webm / avi / mov 等常见格式。
- **智能自动转码**：上传非 H264 视频（如 AV1 / HEVC）自动转码为 H264 MP4，保证兼容性。
- **转码进度条**：转码过程实时显示进度，可随时取消。
- **壁纸预览**：点击「预览」可在管理页直接播放查看效果。
- **壁纸管理**：预置壁纸与用户上传壁纸统一网格展示，支持应用与删除。

### 🖱️ 桌面右键菜单

桌面空白处右键即可快速「设置动态壁纸 / 打开设置」，无需进入控制中心。

### 🧩 预置壁纸

内置 5 个精选动态壁纸视频资源（含 4K 分辨率），开箱即用。

### 🌐 国际化

内置中英文翻译，随系统语言自动适配。

## 使用说明

1. 打开「控制中心 → 个性化 → 动态壁纸」。
2. 打开「启用动态壁纸」开关。
3. 从列表中点击「预览」查看效果，或直接点击「应用」设为壁纸。
4. 点击「选择视频上传」导入本地视频，非 H264 格式将自动转码并显示进度。
5. 桌面空白处右键亦可快速设置动态壁纸。

## 配置

本插件使用 DConfig 统一管理配置，应用 ID 为 `org.deepin.meme`，边缘插件与控制中心插件共享同一套配置数据。

### 配置项一览

| 键 | 类型 | 默认值 | 配置入口 | 说明 |
|---|---|---|---|---|
| `enabled` | bool | `false` | 控制中心开关 | 是否启用动态壁纸 |
| `currentVideo` | string | `""` | 控制中心「应用」 | 当前动态壁纸视频绝对路径 |
| `decodeMode` | string | `"software"` | 配置文件 | 解码模式：`software` / `cuda` / `vaapi` / `auto` |
| `fillMode` | string | `"fill"` | 配置文件 | 铺屏方式：`fill` / `fit` / `stretch` / `center` / `tile` |

其中 `enabled` 与 `currentVideo` 可由控制中心界面直接操作并自动写回配置；`decodeMode` 与 `fillMode` 暂未提供图形化入口，需通过下述配置方式进行设置。

### 配置方式

DConfig 采用「默认值 → 系统级覆盖 → 用户级覆盖」的优先级，默认值由 schema（`/usr/share/dsg/configs/org.deepin.meme/org.deepin.meme.json`）提供，用户配置写入 override 文件。

**用户级配置（仅当前用户生效，推荐）**

```bash
mkdir -p ~/.config/deepin/org.deepin.meme
cat > ~/.config/deepin/org.deepin.meme/override.json <<'EOF'
{
    "decodeMode": "auto",
    "fillMode": "fit"
}
EOF
```

**系统级配置（对所有用户生效，需 root）**

```bash
sudo mkdir -p /etc/dsg/configs/org.deepin.meme
sudo tee /etc/dsg/configs/org.deepin.meme/override.json > /dev/null <<'EOF'
{
    "decodeMode": "vaapi",
    "fillMode": "fill"
}
EOF
```

修改配置后，重启桌面服务使边缘插件重新读取：

```bash
systemctl --user restart dde-shell-plugin@org.deepin.ds.desktop.service
```

> 提示：也可通过 `dconf-editor` 等 DConfig 图形工具查看与修改各键值。

### 配置项详解

**decodeMode（解码模式）**

| 值 | 说明 |
|---|---|
| `software` | 纯软件解码，兼容性最好，CPU 占用高 |
| `cuda` | NVIDIA 独显硬件解码 |
| `vaapi` | 核显 / 通用硬件解码 |
| `auto` | 自动探测：优先 CUDA → VAAPI → 软件解码回退 |

**fillMode（铺屏方式）**

| 值 | 说明 |
|---|---|
| `fill` | 等比放大裁切，铺满屏幕无黑边（默认） |
| `fit` | 完整显示整个画面，可能有黑边 |
| `stretch` | 拉伸至满屏，可能变形 |
| `center` | 原始像素居中显示，不缩放 |
| `tile` | 重复平铺铺满屏幕 |

## 环境要求

| 依赖 | 版本要求 |
|---|---|
| Qt6 | >= 6.8 |
| Dtk6 | >= 6.7 |
| FFmpeg | libavformat / libavcodec / libavutil / libswscale |
| dde-file-manager | 运行依赖 |
| dde-control-center | >= 6.1 |
| ffmpeg（命令行） | 上传转码依赖 |

## 构建与安装

### 源码构建

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
```

### Debian 打包

```bash
dpkg-buildpackage -us -uc -b
```

生成两个安装包：

- `deepin-meme-plugin`：两个插件二进制（边缘插件 + 控制中心插件）。
- `deepin-meme-plugin-data`：预置视频资源与 DConfig schema。

安装后自动重启桌面服务以加载新插件，无需手动操作。

### 安装产物

| 组件 | 路径 |
|---|---|
| 边缘插件 | `/usr/lib/<arch>/dde-file-manager/plugins/desktop-edge/libdd-meme-wallpaper-plugin.so.3.0.0` |
| 控制中心插件 | `/usr/lib/<arch>/dde-control-center/plugins_v1.1/meme/` |
| 预置视频 | `/usr/share/deepin-meme-wallpapers/` |
| DConfig schema | `/usr/share/dsg/configs/org.deepin.meme/org.deepin.meme.json` |

## 更多文档

- [架构设计](./docs/architecture.md)：了解双插件架构与解码渲染链路。
- [README](./README.md)：构建、使用与配置的完整说明。

---

© 2026 UnionTech Software Technology Co., Ltd. 保留所有权利。