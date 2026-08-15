# deepin-meme-plugin

Deepin / UOS v25 动态视频壁纸插件，由两个子插件组成：

- **桌面边缘插件**（`src/desktop-edge`）：基于 FFmpeg 解码视频并嵌入桌面渲染，同时提供桌面右键菜单。
- **控制中心插件**（`src/control-center-plugin`）：在「个性化 → 动态壁纸」中提供启用开关、视频上传、预览与应用等管理界面。

未加锁屏、屏保、性能联动等额外服务，纯插件架构，随 dde-file-manager / dde-control-center 加载。

## 特性

- **FFmpeg 视频解码**：支持 H264 / H265 / AV1 / VP9 等常见编码，软解 + VAAPI + CUDA 硬解三级解码模式。
- **多屏桌面渲染**：视频帧解码后嵌入桌面 root 窗口，多屏共享同一 QPixmap。
- **铺屏方式可配置**：铺满 / 自适应 / 拉伸 / 居中 / 平铺 五种显示模式（DConfig 配置）。
- **桌面右键菜单**：桌面空白处右键可快速「设置动态壁纸 / 打开设置」。
- **控制中心管理页**：启用开关、上传视频、缩略预览、应用/删除壁纸、转码进度条。
- **自动转码**：上传非 H264 视频（如 AV1）自动调用 ffmpeg 转为 H264 MP4，保证壁纸引擎兼容性。
- **跨进程配置共享**：edge 插件与控制中心通过 DConfig（`org.deepin.meme`）共享启用状态与当前壁纸。

## 架构

```
┌─────────────────────────────┐        ┌──────────────────────────────┐
│  dde-control-center          │        │  dde-file-manager (desktop)  │
│  plugins_v1.1/meme/          │        │  plugins/desktop-edge/        │
│                              │        │                              │
│  Meme.qml / MemeMain.qml     │        │  MemeWallpaperPlugin (dpf)   │
│  MemePlugin     ──┐          │        │  WallpaperEngine             │
│  WallpaperModel   │          │        │   ├─ VideoDecoder (QThread)  │
│  VideoConverter   │          │        │   ├─ VideoProxy (QWidget)    │
│                   ▼          │        │   └─ MemeWallpaperMenuScene   │
│            DConfig           │        │              ▲               │
│         org.deepin.meme ◄────┼────────┼──────────────┘               │
└─────────────────────────────┘        └──────────────────────────────┘
```

- **控制中心插件**：QML（DccObject）+ C++ 后端。`VideoConverter` 异步调用 `ffmpeg` 转码并上报进度；`WallpaperModel` 扫描预置视频目录与用户上传目录。
- **边缘插件**：`VideoDecoder` 为独立 QThread 解码线程，按目标帧率节拍输出 `VideoFrame`；`VideoProxy` 为嵌入桌面 root 的 QWidget，负责绘制与 FPS 叠加显示；`WallpaperEngine` 协调开关、资源检查、锁屏/屏保联动。
- **配置**：`MemeConfig` 封装 DConfig 读写，schema 见 `data/configs/org.deepin.meme.json`。

## 目录结构

```
├── CMakeLists.txt                  # 顶层构建：两个子插件 + 预置视频 + schema + 翻译
├── data/
│   ├── configs/org.deepin.meme.json    # DConfig schema（enabled/currentVideo/decodeMode/fillMode）
│   ├── icons/meme_icon.svg             # 控制中心图标
│   └── res/                            # 预置视频（H264 MP4）
├── src/
│   ├── desktop-edge/                   # dde-file-manager 桌面边缘插件
│   │   ├── plugin.cpp/h                # dpf 插件入口（Q_PLUGIN_METADATA）
│   │   ├── engine.cpp/h                # 壁纸引擎：开关/资源/锁屏/屏保联动
│   │   ├── decoder.cpp/h               # FFmpeg 解码线程（软解/VAAPI/CUDA）
│   │   ├── videoproxy.cpp/h            # 嵌入桌面渲染 QWidget + FPS 叠加
│   │   ├── menu.cpp/h                  # 桌面右键菜单场景
│   │   ├── config.cpp/h                # DConfig 读取器
│   │   └── videoframe.h                # 解码帧结构（RGB32 / NV12）
│   └── control-center-plugin/          # dde-control-center 控制中心插件
│       ├── Meme.qml                    # 模块根对象（挂载到 personalization）
│       ├── MemeMain.qml                # 管理页：开关/上传/进度/预览/网格
│       └── src/
│           ├── memeplugin.cpp/h        # 插件业务逻辑 + Q_PROPERTY 暴露给 QML
│           ├── model.cpp/h             # 壁纸列表模型（预置 + 用户上传）
│           └── converter.cpp/h         # H264 格式检测 / ffmpeg 异步转码
├── debian/                             # Debian 打包（rules/control/postinst）
└── translations/deepin-meme-plugin_zh_CN.ts
```

## 构建

### 依赖

| 依赖 | 说明 |
|---|---|
| Qt6 >= 6.8 | Core / Widgets / Gui / DBus / Concurrent / Network / Qml / Multimedia |
| Dtk6 >= 6.7 | Dtk6::Core、Dtk6::Widget |
| FFmpeg | libavformat / libavcodec / libavutil / libswscale |
| dde-file-manager 源码 | 边缘插件依赖 `dfm6-base` / `dfm6-framework` 及头文件 |
| dde-control-center 源码 | 控制中心插件依赖 `libdde-control-center` 及头文件 |

> 注意：`CMakeLists.txt` 顶部的 `DCC_SOURCE_ROOT` 与 `DFM_SOURCE_ROOT` 指向对应仓库的本地源码路径，构建前需按实际情况调整。

### 本地构建

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
```

### Debian 打包

```bash
dpkg-buildpackage -us -uc -b
```

生成两个包：

- `deepin-meme-plugin`：两个插件 `.so`（依赖 `dde-control-center >= 6.1`、`dde-file-manager`、`ffmpeg`）
- `deepin-meme-plugin-data`：预置视频 + DConfig schema

安装后 `postinst` 会自动重启 `dde-shell-plugin@org.deepin.ds.desktop.service` 以加载新边缘插件。

## 安装产物路径

| 组件 | 路径 |
|---|---|
| 边缘插件 | `/usr/lib/<arch>/dde-file-manager/plugins/desktop-edge/libdd-meme-wallpaper-plugin.so.3.0.0` |
| 控制中心插件 | `/usr/lib/<arch>/dde-control-center/plugins_v1.1/meme/meme.so` + `libmeme_qml.so` |
| 预置视频 | `/usr/share/deepin-meme-wallpapers/` |
| DConfig schema | `/usr/share/dsg/configs/org.deepin.meme/org.deepin.meme.json` |
| 翻译文件 | `/usr/share/deepin-meme-plugin/translations/` |

## 使用

1. 打开「控制中心 → 个性化 → 动态壁纸」。
2. 打开「启用动态壁纸」开关。
3. 选择预置壁纸并点击「应用」，或点击「预览」先查看效果。
4. 点击「选择视频上传」导入本地视频（支持 mp4 / mkv / webm / avi / mov），非 H264 格式会自动转码并在上传过程中显示进度条。
5. 用户上传的壁纸可随时「删除」；桌面空白处右键亦可快速「设置动态壁纸」。

## 配置

DConfig 应用 ID 为 `org.deepin.meme`，可用键：

| 键 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `enabled` | bool | `false` | 是否启用动态壁纸 |
| `currentVideo` | string | `""` | 当前动态壁纸视频绝对路径 |
| `decodeMode` | string | `"software"` | 解码模式：`software` / `cuda` / `vaapi` / `auto` |
| `fillMode` | string | `"fill"` | 铺屏方式：`fill` / `fit` / `stretch` / `center` / `tile` |

可在 `/usr/share/dsg/configs/org.deepin.meme/org.deepin.meme.json` 中查看，或通过 `dconf-editor` 等工具修改。

## 常见问题

- **修改源码后打包，边缘插件 `.so` 未更新**：`dh_install` 阶段可能把旧构建的 `.so` 带入包内，重新打包前请清理 `debian/tmp` 与 `debian/deepin-meme-plugin`，并确认 `obj-*/` 为全新构建。
- **更换壁纸后桌面仍显示旧壁纸**：检查 `enabled` 与 `currentVideo` 是否写入成功，卸载/重装后 edge 插件需要重启桌面服务（`postinst` 已自动处理）。

## 许可

本项目基于 [GPL-3.0-or-later](./LICENSE) 许可发布。

- SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
- SPDX-License-Identifier: GPL-3.0-or-later