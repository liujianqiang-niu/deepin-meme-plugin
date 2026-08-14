# Deepin Meme Plugin — 动态壁纸特效插件设计规格

**日期**: 2026-08-14
**项目**: deepin-meme-plugin
**目标系统**: Deepin / UOS v25 (DTK6 / Qt6)
**状态**: 设计完成，用户预批准，进入实现阶段

---

## 1. 概述

### 1.1 目标

在 Deepin/UOS 控制中心"个性化"模块中开发一款插件，让用户设置**带交互特效的动态壁纸**。

设置后，当用户在桌面执行文件操作时，会触发预设的角色动画特效：

- **删除文件/目录** → 奥特曼发激光精准摧毁该文件坐标处的特效
- **新建文件/目录** → 角色诞生/空投动画
- **重命名** → 角色变换/标识动画
- **移动/复制** → 角色搬运/克隆动画

特效以**预制视频/动画包**形式分发，精准定位到被操作文件的**桌面网格坐标**。

### 1.2 核心设计决策

| 决策点 | 选择 | 理由 |
|--------|------|------|
| 特效内容形式 | 预制视频/动画包 | 美术与代码解耦，可扩展主题包 |
| 特效渲染位置 | 局部定位到文件坐标 | "激光打到所以文件没了"的因果感 |
| 触发场景范围 | 全场景（删除/新建/重命名/移动/复制） | 趣味性最大化 |
| 与现有 liveWallpaper 关系 | 独立新插件 | 不破坏现有功能，X11/Wayland 通用 |
| 动态壁纸播放 | 自带 QMediaPlayer 循环播放 | 绕过 liveWallpaper 的 Wayland-only 限制 |
| 预览窗口 | 控制中心内嵌实时预览 | 所见即所得 |
| 运行平台 | X11 + Wayland 双支持 | 特效层为独立 QWidget 叠加，平台无关 |

### 1.3 范围边界

**第一阶段交付（MVP 骨架）**:
- ✅ 控制中心插件骨架（DccObject 注册 + 预览窗口 + 设置界面）
- ✅ 桌面特效层骨架（EffectOverlay widget + 信号订阅 + 坐标映射）
- ✅ 资源包格式定义与加载器
- ✅ 可编译的 CMake 工程
- ✅ 一个示例主题包结构

**第二阶段交付（功能完善）**:
- ✅ DConfig 持久化集成（MemeDConfig 封装 DTK6 DConfig）
- ✅ 多显示器支持（QScreen 遍历 + 几何偏移计算）
- ✅ 精确坐标 D-Bus 查询（org.deepin.dde.desktop.canvas 前瞻性调用 + 屏幕中心回退）
- ✅ 性能优化（去抖 500ms + 并发限制）
- ✅ 重命名/移动/复制事件检测（inode 对比 + 快照差分）
- ✅ 占位演示视频资源（ffmpeg 程序化生成,无版权问题）

**不在范围（需用户自行获取或后续迭代）**:
- 正式美术资源（奥特曼/僵尸的授权视频——见第 11 节资源获取指南）
- 主题包市场/在线下载
- 生产级性能调优

---

## 2. 架构设计

### 2.1 系统架构总览

```
┌─────────────────────────────────────────────────────────┐
│                     控制中心 (dde-control-center)        │
│  ┌─────────────────────────────────────────────────────┐ │
│  │  个性化模块 (plugin-personalization)                │ │
│  │  ├── 壁纸 (wallpaper)                               │ │
│  │  ├── 主题 (theme)                                   │ │
│  │  └── ...                                            │ │
│  └─────────────────────────────────────────────────────┘ │
│  ┌─────────────────────────────────────────────────────┐ │
│  │  ★ 新增: 趣味壁纸插件 (plugin-meme)                  │ │
│  │  ├── 设置界面 (MemeSettings.qml)                    │ │
│  │  ├── 预览窗口 (MemePreview.qml)                     │ │
│  │  ├── 主题包选择 (MemeThemeSelect.qml)               │ │
│  │  └── C++ 后端 (MemePlugin)                          │ │
│  └─────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
                         │ D-Bus 配置同步
                         ▼
┌─────────────────────────────────────────────────────────┐
│              桌面进程 (dde-shell → panel-desktop)       │
│  ┌─────────────────────────────────────────────────────┐ │
│  │  BaseWindow (Layer 0.0) — 透明根窗口                 │ │
│  │  ┌───────────────────────────────────────────────┐  │ │
│  │  │  BackgroundDefault (Layer 5.0) — 壁纸层       │  │ │
│  │  │  ★ 被特效插件替换为循环视频壁纸                  │  │ │
│  │  └───────────────────────────────────────────────┘  │ │
│  │  ┌───────────────────────────────────────────────┐  │ │
│  │  │  CanvasView (Layer 10.0) — 桌面图标            │  │ │
│  │  └───────────────────────────────────────────────┘  │ │
│  │  ┌───────────────────────────────────────────────┐  │ │
│  │  │  ★ EffectOverlay (Layer 15.0) — 特效层(新增)   │  │ │
│  │  │  透明 QWidget, 精准定位到文件坐标              │  │ │
│  │  │  订阅 FileProvider::fileRemoved 等信号          │  │ │
│  │  └───────────────────────────────────────────────┘  │ │
│  └─────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

### 2.2 组件职责

#### 2.2.1 控制中心插件 (plugin-meme)

**职责**: 提供设置界面，让用户选择特效主题包、预览效果、开关功能。

**入口**: DccObject 注册到 `personalization` 下，name=`meme`。

**模块树**:
```
personalization
└── meme (weight: 350, 在 wallpaper 之后)
    ├── memeTitle
    ├── memeEnabled (开关)
    ├── memeTheme (主题包选择)
    │   └── MemeThemeSelect (网格选择器)
    ├── memePreview (实时预览)
    │   └── MemePreview (QMediaPlayer + QVideoWidget)
    └── memeSettings (高级设置)
        ├── effectVolume (音效音量)
        └── effectDuration (特效时长)
```

**C++ 后端** (`MemePlugin`):
- `Q_PROPERTY` 暴露给 QML: `enabled`, `currentTheme`, `themeList`, `previewVideo`
- `Q_INVOKABLE` 方法: `setTheme(QString)`, `previewEffect(QString eventType)`, `setEnabled(bool)`
- 通过 D-Bus 与桌面特效层通信（同步配置）

#### 2.2.2 桌面特效层 (EffectOverlay)

**职责**: 在桌面上叠加透明窗口，监听文件操作信号，在文件坐标处播放特效动画。

**注册方式**: 作为 dde-file-manager 的 DPF 插件 `ddplugin-meme`，或作为独立守护进程通过 D-Bus 监听。MVP 采用**独立守护进程**方案（降低耦合，不改动 dde-file-manager 源码）。

**信号订阅**:

| 操作 | DPF 事件 | D-Bus 信号 | 守护进程订阅 |
|------|----------|-----------|-------------|
| 删除 | `kMoveToTrashResult` | — | ✅ |
| 永久删除 | `kDeleteFilesResult` | — | ✅ |
| 新建 | `rowsInserted` | — | ✅ |
| 重命名 | `dataReplaced` | — | ✅ |
| 移动/复制 | `kCopyFileResult` / `kCutFileResult` | — | ✅ |

**MVP 守护进程方案**: 通过监听文件系统 + D-Bus 获取桌面文件变更，通过 `com.deepin.dde.desktop` D-Bus 接口获取桌面几何信息。精确坐标获取通过 D-Bus 调用 `org.deepin.dde.desktop.canvas` 接口（需扩展，或通过文件 URL 在桌面视图中查找）。

#### 2.2.3 资源包格式

每个特效主题包是一个目录：

```
/usr/share/deepin-meme-themes/ultraman/
├── manifest.json          # 主题元数据
├── wallpaper.mp4           # 循环播放的动态壁纸
├── wallpaper.thumbnail.jpg # 预览缩略图
├── effects/                # 各操作对应的特效视频
│   ├── delete.webm          # 删除特效 (带 alpha 通道)
│   ├── delete.wav           # 删除音效
│   ├── create.webm          # 新建特效
│   ├── create.wav
│   ├── rename.webm          # 重命名特效
│   ├── rename.wav
│   ├── move.webm             # 移动特效
│   ├── move.wav
│   ├── copy.webm             # 复制特效
│   └── copy.wav
└── config.json             # 动画参数(锚点、时长、缩放)
```

**manifest.json 格式**:
```json
{
  "id": "ultraman",
  "name": "奥特曼激光特效",
  "description": "奥特曼发激光摧毁文件",
  "version": "1.0.0",
  "author": "Deepin Meme Team",
  "wallpaper": "wallpaper.mp4",
  "thumbnail": "wallpaper.thumbnail.jpg",
  "effects": {
    "delete": {
      "video": "effects/delete.webm",
      "audio": "effects/delete.wav",
      "anchor": "target",
      "scale": 1.0,
      "duration": 3000
    },
    "create": {
      "video": "effects/create.webm",
      "audio": "effects/create.wav",
      "anchor": "target",
      "scale": 0.8,
      "duration": 2000
    },
    "rename": { "video": "effects/rename.webm", "audio": "effects/rename.wav" },
    "move": { "video": "effects/move.webm", "audio": "effects/move.wav" },
    "copy": { "video": "effects/copy.webm", "audio": "effects/copy.wav" }
  }
}
```

**anchor 字段说明**:
- `"target"` — 特效中心对齐到被操作文件的坐标
- `"screen-center"` — 特效居中于屏幕
- `"corner-tl"` — 左上角

---

## 3. 技术栈与依赖

### 3.1 已安装（无需安装）

| 依赖 | 版本 | 用途 |
|------|------|------|
| Qt6 Core/Gui/Widgets | 6.8.0 | 基础框架 |
| Qt6 Declarative/QML | 6.8.0 | 控制中心插件 QML |
| Qt6 Multimedia | 6.8.0 | 视频壁纸播放 + 音效 |
| Qt6 DBus | 6.8.0 | 进程间通信 |
| Qt6 Svg | 6.8.0 | 图标 |
| DTK6 Core/Gui/Widget/Declarative | 6.7.44 | DTK 控件与主题 |
| cmake | 3.31.4 | 构建系统 |
| g++ | 12.3.0 | 编译器 (C++17) |

### 3.2 需要安装的包

**无需额外安装。** 所有依赖已在系统中就绪。

`dde-control-center` (6.1.96) 和 `dde-desktop` (6.5.150) 运行时已安装，插件开发只需 dev 包（已确认 DTK6 dev 全套和 Qt6 dev 全套已装）。

如果后续需要 DPF 插件开发（直接集成到 dde-file-manager），则需安装 `libdframeworksdev-dev`，但 MVP 采用独立守护进程方案，暂不需要。

---

## 4. 项目结构

```
deepin-meme-plugin/
├── CMakeLists.txt                    # 顶层 CMake
├── docs/
│   └── superpowers/specs/
│       └── 2026-08-14-deepin-meme-plugin-design.md  # 本文档
├── src/
│   ├── control-center-plugin/         # 控制中心插件
│   │   ├── CMakeLists.txt
│   │   ├── qml/
│   │   │   ├── Meme.qml               # 模块根 DccObject
│   │   │   ├── MemeMain.qml           # 主页面
│   │   │   ├── MemePreview.qml        # 预览窗口
│   │   │   └── MemeThemeSelect.qml    # 主题选择
│   │   └── src/
│   │       ├── memeplugin.h           # DccFactory 后端
│   │       ├── memeplugin.cpp
│   │       ├── memethememanager.h     # 主题包管理
│   │       ├── memethememanager.cpp
│   │       ├── memepreviewplayer.h    # 预览播放器
│   │       └── memepreviewplayer.cpp
│   ├── desktop-daemon/                # 桌面特效守护进程
│   │   ├── CMakeLists.txt
│   │   └── src/
│   │       ├── main.cpp               # 守护进程入口
│   │       ├── effectoverlay.h        # 特效叠加层
│   │       ├── effectoverlay.cpp
│   │       ├── fileoperationmonitor.h # 文件操作监听
│   │       ├── fileoperationmonitor.cpp
│   │       ├── themeresolver.h        # 主题包加载与解析
│   │       ├── themeresolver.cpp
│   │       ├── effectplayer.h          # 特效播放(QMediaPlayer)
│   │       └── effectplayer.cpp
│   └── common/                        # 共享代码
│       ├── CMakeLists.txt
│       └── src/
│           ├── thememanifest.h         # manifest.json 数据结构
│           ├── thememanifest.cpp
│           ├── memeconfig.h            # DConfig 键定义
│           └── memeconfig.cpp
├── data/
│   ├── configs/
│   │   └── org.deepin.meme.json       # DConfig schema
│   └── themes/
│       └── example/                   # 示例主题包
│           ├── manifest.json
│           └── ... 
└── translations/                      # 国际化
    └── deepin-meme-plugin.ts
```

---

## 5. 关键流程

### 5.1 设置壁纸与特效流程

```
用户打开控制中心 → 个性化 → 趣味壁纸
  → 选择主题包 (MemeThemeSelect)
  → 预览窗口实时播放壁纸 + 演示特效 (MemePreview)
  → 点击"应用"
  → MemePlugin 写入 DConfig (org.deepin.meme)
  → D-Bus 通知桌面守护进程 (org.deepin.meme.daemon)
  → 守护进程加载主题包，替换桌面壁纸渲染，准备特效层
```

### 5.2 特效触发流程（以删除为例）

```
用户在桌面删除文件 "report.docx"
  → FileProvider::fileRemoved(QUrl("file://.../Desktop/report.docx"))
  → 文件系统监视器 (守护进程中的 LocalFileWatcher) 检测到删除
  → FileOperationMonitor 识别为删除操作
  → 查询文件桌面坐标:
    → 通过 com.deepin.dde.desktop.canvas D-Bus 获取 (MVP)
    → 或通过 GeometryCalculator 根据网格算法计算 (fallback)
  → ThemeResolver 根据当前主题加载 delete.webm + delete.wav
  → EffectOverlay 在文件坐标处创建透明窗口
  → EffectPlayer 播放视频 + 音效
  → 动画完成 → EffectOverlay 关闭窗口
```

### 5.3 坐标获取策略

**MVP 策略（守护进程独立运行，不依赖 DPF 插件）**:

由于独立守护进程无法直接访问 `CanvasGrid::point()`，采用以下策略：

1. **监听桌面文件目录** (`~/Desktop`) 的文件变更
2. 文件被删除时，文件已不在，无法获取其原桌面坐标
3. **解决方案**: 守护进程启动时扫描 `~/Desktop`，记录每个文件的桌面位置估算（根据网格布局算法）
4. 或更简单：**特效播放位置使用屏幕中心**作为 MVP fallback，后续版本通过 D-Bus 扩展获取精确坐标

**精确坐标方案（后续版本）**:
- 扩展 `org.deepin.dde.desktop.canvas` D-Bus 接口，新增 `GetItemRect(QString url)` 方法
- 或开发 `ddplugin-meme` DPF 插件，直接访问 `CanvasGrid` 和 `CanvasView`

**MVP 务实选择**: 特效在屏幕中心区域播放，同时通过 D-Bus 通知桌面（后续迭代）。文档中保留精确坐标的技术路径，但 MVP 先跑通完整链路。

---

## 6. D-Bus 接口设计

### 6.1 守护进程服务

**服务名**: `org.deepin.meme.daemon`
**对象路径**: `/org/deepin/meme/daemon`
**接口**: `org.deepin.meme.daemon`

```
方法:
  SetEnabled(bool enabled)                    → 开关特效
  SetTheme(QString themeId)                   → 切换主题包
  GetTheme() → QString                        → 获取当前主题
  GetThemes() → QStringList                   → 获取可用主题列表
  PreviewEffect(QString effectType)           → 预览指定特效
  SetEffectVolume(int volume)                 → 设置音效音量

信号:
  ThemeChanged(QString themeId)               → 主题切换通知
  EffectTriggered(QString effectType, int x, int y)  → 特效触发通知
```

### 6.2 控制中心插件通信

控制中心插件通过 QDBus 调用守护进程接口，实现配置同步。

---

## 7. DConfig 配置

**Schema 文件**: `data/configs/org.deepin.meme.json`

```json
{
  "name": "meme",
  "owner": "org.deepin.meme",
  "version": "1.0.0",
  "items": {
    "enabled": {
      "type": "boolean",
      "default": false,
      "description": "Whether meme wallpaper effects are enabled"
    },
    "currentTheme": {
      "type": "string",
      "default": "example",
      "description": "Current meme theme id"
    },
    "effectVolume": {
      "type": "integer",
      "default": 80,
      "description": "Effect audio volume (0-100)"
    }
  }
}
```

---

## 8. 构建与部署

### 8.1 构建

```bash
cd deepin-meme-plugin
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### 8.2 安装

```bash
sudo cmake --install .
```

**安装产物**:
- `/usr/lib/dde-control-center/plugins_v1.1/meme/meme.so` — 控制中心插件
- `/usr/lib/dde-control-center/plugins_v1.1/meme/qmldir` — QML 模块清单
- `/usr/lib/dde-control-center/plugins_v1.1/meme/Meme.qml` 等 QML 文件
- `/usr/bin/deepin-meme-daemon` — 桌面特效守护进程
- `/usr/share/dconfig/meta/org.deepin.meme.json` — DConfig schema
- `/usr/share/deepin-meme-themes/example/` — 示例主题包
- `/usr/share/dbus-1/services/org.deepin.meme.daemon.service` — D-Bus 服务文件

### 8.3 运行

1. 控制中心自动加载 `meme` 插件（通过 DccPluginLoader 扫描 `plugins_v1.1`）
2. 桌面特效守护进程通过 D-Bus 服务自动激活（session bus）

---

## 9. 测试策略

- **单元测试**: ThemeManifest 解析、MemeConfig 读写、坐标计算
- **集成测试**: D-Bus 接口调用、插件加载
- **手动验证**: 控制中心显示插件页面、预览播放、删除文件触发特效

---

## 10. 风险与缓解

| 风险 | 缓解 |
|------|------|
| 文件坐标获取困难（守护进程无法访问 CanvasGrid） | MVP 用屏幕中心，后续扩展 D-Bus 接口 |
| 视频 alpha 通道在 X11 下支持差 | 用 webm 格式，或 fallback 到 gif/逐帧 PNG |
| 特效层可能阻挡桌面点击 | 透明窗口设置 `Qt::WA_TransparentForMouseEvents` 或仅在动画期间显示 |
| 性能（视频解码占用） | 限制特效时长 ≤3s，预解码缓存 |
| 版权（奥特曼/僵尸素材） | MVP 用占位示例主题，正式素材需授权 |

---

## 11. 资源获取指南

### 11.1 为什么不直接下载奥特曼/植物大战僵尸素材

奥特曼（Ultraman）是圆谷制作株式会社的注册商标，植物大战僵尸（Plants vs Zombies）是 EA/PopCap 的版权作品。未经授权下载、分发、二次创作这些素材用于公开项目构成版权侵权，即使个人使用也有法律风险。

### 11.2 合法免费视频特效资源站

以下站点提供 CC0 或免版税(Royalty-free)的特效视频,可合法用于本项目:

| 站点 | 许可证 | 特点 | URL |
|------|--------|------|-----|
| FreeVisuals | Free for personal & commercial | 绿幕爆炸特效 | https://www.freevisuals.net |
| Vidiots Channel | Free for personal videos | 绿幕火焰/爆炸 | https://www.vidiotschannel.com |
| mycreativefx | Royalty-free | 4K 绿幕 VFX 包 | https://mycreativefx.com |
| Vecteezy | Free License (Attribution) | 4K 爆炸动画 | https://www.vecteezy.com |
| Mirin's Stock | Free for personal & commercial | 卡通风格爆炸(适合趣味主题) | https://miirriin.com |
| Pexels | CC0 | 通用视频素材 | https://www.pexels.com |
| Pixabay | CC0 | 通用视频素材 | https://pixabay.com |
| Mixkit | Free | VFX 与背景视频 | https://mixkit.co |

### 11.3 获取正式素材的合法途径

1. **购买授权**: 在 Artlist、Epidemic Sound、Motion Array、Envato Elements 等站购买订阅,获取商业授权的特效素材
2. **自行制作**: 使用 Blender(开源 3D)、After Effects、DaVinci Resolve 等工具自行制作角色动画
3. **委托创作**: 在 Fiverr、Upwork 等平台委托动画师制作原创角色(需明确版权归属)
4. **开源角色**: 使用 CC0 或 CC-BY 许可的原创角色(如 OpenGameArt 上的作品)

### 11.4 主题包制作流程

1. 从上述站点下载特效视频(优先选择带 alpha 通道的 webm/mov,或绿幕 mp4 需用 chroma key 处理)
2. 转换为 webm(VP9 + yuva420p)以支持透明背景:
   ```bash
   ffmpeg -i input.mp4 -c:v libvpx-vp9 -pix_fmt yuva420p -auto-alt-ref 0 output.webm
   ```
3. 按主题包目录结构组织:
   ```
   /usr/share/deepin-meme-themes/mytheme/
   ├── manifest.json
   ├── wallpaper.mp4
   ├── wallpaper.thumbnail.jpg
   └── effects/
       ├── delete.webm + delete.wav
       ├── create.webm + create.wav
       ├── rename.webm + rename.wav
       ├── move.webm + move.wav
       └── copy.webm + copy.wav
   ```
4. 重启守护进程或调用 `SetTheme` D-Bus 方法切换主题

### 11.5 已生成的占位演示资源

项目已附带 ffmpeg 程序化生成的占位演示视频(位于 `data/themes/example/`):
- `wallpaper.mp4` (7.4M, 10秒 1920x1080 循环)
- 5 个特效 webm(带 alpha 通道): delete/create/rename/move/copy
- 5 个音效 wav
- 生成脚本: `scripts/generate-demo-videos.sh`

这些占位资源无版权限制,可直接用于测试和演示。
