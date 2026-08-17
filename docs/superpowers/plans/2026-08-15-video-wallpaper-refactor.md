# 动态壁纸重构实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 将 deepin-meme-plugin 从独立 daemon + 顶层窗口重构为 dde-file-manager desktop-edge 插件 + 控制中心插件 + DConfig 共享配置，并增加用户上传视频与自动格式转换功能。

**架构：** 三组件：(1) desktop-edge 插件嵌入桌面框架用 FFmpeg 解码渲染视频壁纸 + 右键菜单；(2) 控制中心插件提供 UI 管理和视频上传/转码；(3) DConfig schema 跨进程共享配置。参考实现：`<your-workspace>/deepin_video_wallpaper`。

**技术栈：** Qt6 Widgets, DTK6 Core, FFmpeg (libavformat/libavcodec/libavutil/libswscale), dde-file-manager dfm-framework (dpf::Plugin), dde-control-center dccfactory, DConfig, QProcess (ffmpeg CLI)

**关键源码路径：**
- dde-file-manager 源码: `<your-workspace>/dde-file-manager`（include 路径）
- dde-control-center 源码: `<your-workspace>/dde-control-center`（include 路径）
- 参考实现: `<your-workspace>/deepin_video_wallpaper`

**DCC_SOURCE_ROOT**: `<your-workspace>/dde-control-center`
**DFM_SOURCE_ROOT**: `<your-workspace>/dde-file-manager`

---

## 文件结构

### 创建的文件

| 文件 | 职责 |
|------|------|
| `src/desktop-edge/CMakeLists.txt` | edge 插件构建 |
| `src/desktop-edge/meme_videowallpaper.json` | 插件元数据 |
| `src/desktop-edge/global.h` | 命名空间 + 日志分类 |
| `src/desktop-edge/plugin.h/cpp` | dpf::Plugin 入口 |
| `src/desktop-edge/engine.h/cpp` | WallpaperEngine 核心 |
| `src/desktop-edge/engine_p.h` | WallpaperEngine 私有类 |
| `src/desktop-edge/videoproxy.h/cpp` | 嵌入桌面 QWidget + QPainter |
| `src/desktop-edge/decoder.h/cpp` | FFmpeg 解码线程 |
| `src/desktop-edge/videoframe.h` | 帧数据结构 |
| `src/desktop-edge/menu.h/cpp` | 右键菜单场景 |
| `src/desktop-edge/config.h/cpp` | DConfig 读取器 |
| `src/desktop-edge/util/event_helper.h` | dpf 事件助手 |
| `src/control-center-plugin/src/model.h/cpp` | 壁纸列表 QAbstractListModel |
| `src/control-center-plugin/src/converter.h/cpp` | ffmpeg 格式检查/转码 |

### 修改的文件

| 文件 | 变更 |
|------|------|
| `CMakeLists.txt` | 顶层重构：移除 daemon/common，加 desktop-edge/control-center |
| `data/configs/org.deepin.meme.json` | 新增 decodeMode, fillMode 键 |
| `src/control-center-plugin/CMakeLists.txt` | 移除 QML 依赖，加 Concurrent/Network |
| `src/control-center-plugin/src/memeplugin.h/cpp` | 重写：用 DConfig + Model + Converter |
| `src/control-center-plugin/MemeMain.qml` | 重写：壁纸网格 + 上传 + 转码进度 |
| `debian/control` | 依赖改为 FFmpeg + dde-file-manager |
| `debian/rules` | 构建配置 |
| `debian/deepin-meme-plugin.install` | 安装路径 |
| `debian/postinst` | 重启桌面插件 |

### 删除的文件

| 文件 | 原因 |
|------|------|
| `src/desktop-daemon/` (全部) | 独立 daemon 废弃 |
| `src/common/` (全部) | MemedConfig 被 DConfig 替代 |
| `data/systemd/deepin-meme-daemon.service` | systemd service 废弃 |
| `data/dbus/org.deepin.meme.daemon.service` | D-Bus service 废弃 |
| `data/res/transparent.png` | 不再需要透明壁纸 hack |

---

## 任务 1：清理旧代码 + 更新顶层构建

**文件：**
- 删除：`src/desktop-daemon/`, `src/common/`, `data/systemd/`, `data/dbus/`, `data/res/transparent.png`
- 修改：`CMakeLists.txt`

- [ ] **步骤 1：删除旧 daemon 代码和过时资源**

```bash
rm -rf src/desktop-daemon src/common
rm -f data/systemd/deepin-meme-daemon.service data/dbus/org.deepin.meme.daemon.service
rm -f data/res/transparent.png
rmdir data/systemd data/dbus 2>/dev/null || true
```

- [ ] **步骤 2：重写顶层 CMakeLists.txt**

```cmake
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.23)

project(deepin-meme-plugin
    VERSION 3.0.0
    DESCRIPTION "Deepin dynamic wallpaper plugin"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(DCC_SOURCE_ROOT "<your-workspace>/dde-control-center")
set(DCC_INCLUDE_DIR "${DCC_SOURCE_ROOT}/include")
set(DCC_PLUGIN_DIR "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/dde-control-center/plugins_v1.1")
set(DFM_SOURCE_ROOT "<your-workspace>/dde-file-manager")
set(DFM_INCLUDE_DIR "${DFM_SOURCE_ROOT}/include")
set(EDGE_PLUGIN_DIR "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/dde-file-manager/plugins/desktop-edge")
set(MEME_WALLPAPER_DIR "/usr/share/deepin-meme-wallpapers")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui DBus Concurrent Network LinguistTools)
find_package(Dtk6 REQUIRED COMPONENTS Core)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale)
find_library(DFM6_BASE_LIB dfm6-base REQUIRED)
find_library(DFM6_FRAMEWORK_LIB dfm6-framework REQUIRED)

add_subdirectory(src/desktop-edge)
add_subdirectory(src/control-center-plugin)

# 预置视频
install(DIRECTORY data/res/
    DESTINATION ${MEME_WALLPAPER_DIR}
    FILES_MATCHING PATTERN "*.mp4")

# DConfig schema
dtk_add_config_meta_files(APPID org.deepin.meme FILES data/configs/org.deepin.meme.json)

# 翻译
qt_add_lupdate(LTS_FILES
    SOURCE_TARGETS meme_qml
    TS_FILES translations/deepin-meme-plugin_zh_CN.ts)
qt_add_lrelease(QM_FILES
    TS_FILES translations/deepin-meme-plugin_zh_CN.ts)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/deepin-meme-plugin_zh_CN.qm
    DESTINATION /usr/share/deepin-meme-plugin/translations)

# 图标
install(FILES data/icons/meme_icon.svg
    DESTINATION /usr/share/icons/hicolor/scalable/apps)
```

- [ ] **步骤 3：验证配置阶段**

运行：`cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr 2>&1 | tail -10`
预期：配置可能报错（子目录 CMakeLists 尚未更新），但顶层变量设置正确

- [ ] **步骤 4：Commit**

```bash
git add -A
git commit -m "refactor: remove daemon, restructure for edge plugin architecture

Removed: desktop-daemon, common (MemeDConfig), systemd/dbus services, transparent.png.
Rewrote top-level CMakeLists for desktop-edge + control-center two-component build.

refactor: 移除 daemon，重构为 edge 插件架构"
```

---

## 任务 2：更新 DConfig Schema

**文件：**
- 修改：`data/configs/org.deepin.meme.json`

- [ ] **步骤 1：重写 DConfig schema**

```json
{
    "magic": "dsg.config.meta",
    "version": "1.0",
    "contents": {
        "enabled": {
            "value": false,
            "serial": 0,
            "flags": [],
            "name": "enable dynamic wallpaper",
            "name[zh_CN]": "启用动态壁纸",
            "description": "Whether dynamic wallpaper is enabled",
            "description[zh_CN]": "是否启用动态壁纸",
            "permissions": "readwrite",
            "visibility": "public",
            "type": "bool"
        },
        "currentVideo": {
            "value": "",
            "serial": 0,
            "flags": [],
            "name": "current video wallpaper path",
            "name[zh_CN]": "当前动态壁纸路径",
            "description": "Absolute path to the current video wallpaper file",
            "description[zh_CN]": "当前动态壁纸视频文件绝对路径",
            "permissions": "readwrite",
            "visibility": "public",
            "type": "string"
        },
        "decodeMode": {
            "value": "software",
            "serial": 0,
            "flags": [],
            "name": "decode mode",
            "name[zh_CN]": "解码模式",
            "description": "software/cuda/vaapi/auto",
            "description[zh_CN]": "软解/CUDA/VAAPI/自动",
            "permissions": "readwrite",
            "visibility": "public",
            "type": "string"
        },
        "fillMode": {
            "value": "fill",
            "serial": 0,
            "flags": [],
            "name": "fill mode",
            "name[zh_CN]": "铺屏方式",
            "description": "fill/fit/stretch/center/tile",
            "description[zh_CN]": "铺满/自适应/拉伸/居中/平铺",
            "permissions": "readwrite",
            "visibility": "public",
            "type": "string"
        }
    }
}
```

- [ ] **步骤 2：Commit**

```bash
git add data/configs/org.deepin.meme.json
git commit -m "feat(dconfig): add decodeMode and fillMode keys for edge plugin"
```

---

## 任务 3：desktop-edge 插件骨架

**文件：**
- 创建：`src/desktop-edge/CMakeLists.txt`, `meme_videowallpaper.json`, `global.h`, `plugin.h`, `plugin.cpp`, `util/event_helper.h`

- [ ] **步骤 1：创建插件元数据 JSON**

`src/desktop-edge/meme_videowallpaper.json`:
```json
{
    "Name" : "dd-meme-wallpaper",
    "Version" : "3.0.0",
    "CompatVersion" : "3.0.0",
    "Vendor" : "UnionTech Software Technology Co., Ltd.",
    "Copyright" : "Copyright (C) 2026 UnionTech Software Technology Co., Ltd.",
    "License" : ["GPL-3.0-or-later"],
    "Category" : "",
    "Description" : "Dynamic video wallpaper plugin for Deepin desktop.",
    "UrlLink" : "https://www.uniontech.com",
    "Depends" : [
        {"Name" : "ddplugin-core", "Version": "1.0.0"}
    ]
}
```

- [ ] **步骤 2：创建 global.h**

`src/desktop-edge/global.h`:
```cpp
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef DD_MEME_WALLPAPER_GLOBAL_H
#define DD_MEME_WALLPAPER_GLOBAL_H

#include <dfm-base/dfm_log_defines.h>

#define DD_MEME_WALLPAPER_NAMESPACE ddplugin_meme

#define DD_MEME_WALLPAPER_BEGIN_NAMESPACE namespace DD_MEME_WALLPAPER_NAMESPACE{
#define DD_MEME_WALLPAPER_END_NAMESPACE }
#define DD_MEME_WALLPAPER_USE_NAMESPACE using namespace DD_MEME_WALLPAPER_NAMESPACE;

DD_MEME_WALLPAPER_BEGIN_NAMESPACE
DFM_LOG_USE_CATEGORY(DD_MEME_WALLPAPER_NAMESPACE)
DD_MEME_WALLPAPER_END_NAMESPACE

#endif
```

- [ ] **步骤 3：创建 event_helper.h**

`src/desktop-edge/util/event_helper.h` — 直接从参考实现复制 `ddpugin_eventinterface_helper.h`，适配命名空间。提供 `desktopFrameRootWindows()` 等内联函数。

```cpp
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEME_EVENT_HELPER_H
#define MEME_EVENT_HELPER_H

#include <dfm-base/interfaces/screen/abstractscreenproxy.h>
#include <dfm-framework/dpf.h>

#define CanvasCorePush(topic) dpfSlotChannel->push("ddplugin_core", QT_STRINGIFY2(topic))
#define CanvasCorePush2(topic, args...) dpfSlotChannel->push("ddplugin_core", QT_STRINGIFY2(topic), ##args)

namespace ddplugin_meme_util {
static inline QList<QWidget *> desktopFrameRootWindows() {
    const QVariant &ret = CanvasCorePush(slot_DesktopFrame_RootWindows);
    return ret.value<QList<QWidget *>>();
}
}
#endif
```

- [ ] **步骤 4：创建 plugin.h**

`src/desktop-edge/plugin.h`:
```cpp
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEME_WALLPAPER_PLUGIN_H
#define MEME_WALLPAPER_PLUGIN_H

#include "global.h"
#include <dfm-framework/dpf.h>

namespace DD_MEME_WALLPAPER_NAMESPACE {
class WallpaperEngine;
class MemeWallpaperPlugin : public dpf::Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.deepin.plugin.desktop" FILE "meme_videowallpaper.json")
public:
    explicit MemeWallpaperPlugin(QObject *parent = nullptr);
    void initialize() override;
    bool start() override;
    void stop() override;
private:
    WallpaperEngine *engine = nullptr;
};
}
#endif
```

- [ ] **步骤 5：创建 plugin.cpp**

`src/desktop-edge/plugin.cpp`:
```cpp
// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "engine.h"
#include <QDebug>

DD_MEME_WALLPAPER_USE_NAMESPACE

DD_MEME_WALLPAPER_BEGIN_NAMESPACE
DFM_LOG_REISGER_CATEGORY(DD_MEME_WALLPAPER_NAMESPACE)
DD_MEME_WALLPAPER_END_NAMESPACE

MemeWallpaperPlugin::MemeWallpaperPlugin(QObject *parent) : Plugin() { Q_UNUSED(parent) }

void MemeWallpaperPlugin::initialize() {}

bool MemeWallpaperPlugin::start()
{
    engine = new WallpaperEngine();
    return engine->init();
}

void MemeWallpaperPlugin::stop()
{
    delete engine;
    engine = nullptr;
}
```

- [ ] **步骤 6：创建 CMakeLists.txt**

`src/desktop-edge/CMakeLists.txt`:
```cmake
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
set(EDGE_SOURCES
    plugin.cpp
    engine.cpp
    videoproxy.cpp
    decoder.cpp
    menu.cpp
    config.cpp
)

add_library(dd-meme-wallpaper SHARED ${EDGE_SOURCES} meme_videowallpaper.json)

set_target_properties(dd-meme-wallpaper PROPERTIES
    OUTPUT_NAME "dd-meme-wallpaper-plugin"
    PREFIX "lib"
    VERSION ${PROJECT_VERSION}
    SOVERSION 3
)

target_include_directories(dd-meme-wallpaper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/util
    ${DFM_INCLUDE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(dd-meme-wallpaper PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::DBus
    Dtk6::Core
    ${DFM6_BASE_LIB}
    ${DFM6_FRAMEWORK_LIB}
    ${FFMPEG_LIBRARIES}
)

target_link_options(dd-meme-wallpaper PRIVATE -Wl,--no-undefined)

install(TARGETS dd-meme-wallpaper
    LIBRARY DESTINATION ${EDGE_PLUGIN_DIR}
    NAMELINK_SKIP
)
```

- [ ] **步骤 7：构建验证（预期失败，因为 engine 等尚未创建）**

运行：`cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr 2>&1 | tail -5`
预期：配置通过但构建报错缺文件

- [ ] **步骤 8：Commit**

```bash
git add src/desktop-edge/
git commit -m "feat(edge): add desktop-edge plugin skeleton

Plugin entry with dpf::Plugin, metadata JSON, global.h, event helpers.
Based on deepin_video_wallpaper reference implementation."
```

---

## 任务 4：VideoFrame + VideoDecoder（FFmpeg 解码线程）

**文件：**
- 创建：`src/desktop-edge/videoframe.h`, `src/desktop-edge/decoder.h`, `src/desktop-edge/decoder.cpp`

**参考：** 直接从 `<your-workspace>/deepin_video_wallpaper/src/videoframe.h` 和 `videodecoder.h/cpp` 复制并适配命名空间。

- [ ] **步骤 1：创建 videoframe.h**

从参考实现 `videoframe.h` 复制，将命名空间从 `ddplugin_videowallpaper` 改为 `ddplugin_meme`。

- [ ] **步骤 2：创建 decoder.h**

从参考实现 `videodecoder.h` 复制，适配命名空间，移除 `DecodeMode`（改从 DConfig 读取字符串转枚举）。

- [ ] **步骤 3：创建 decoder.cpp**

从参考实现 `videodecoder.cpp` 复制全量代码（412行），适配命名空间。这是 FFmpeg 解码线程的核心，包含：
- `playOne()`: avformat_open_input → avcodec_decode → sws_scale BGRA → emit frameReady
- `run()`: 循环播放 playlist
- CUDA/VAAPI 硬解探测 + 回退
- 帧率控制 + 丢帧 + 双缓冲

- [ ] **步骤 4：Commit**

```bash
git add src/desktop-edge/videoframe.h src/desktop-edge/decoder.h src/desktop-edge/decoder.cpp
git commit -m "feat(edge): add FFmpeg VideoDecoder thread

QThread-based FFmpeg decoder with CUDA/VAAPI/Software support.
Adapted from deepin_video_wallpaper reference implementation."
```

---

## 任务 5：VideoProxy（嵌入桌面 QWidget）

**文件：**
- 创建：`src/desktop-edge/videoproxy.h`, `src/desktop-edge/videoproxy.cpp`

**参考：** 从参考实现 `videoproxy.h/cpp` 复制并适配。

- [ ] **步骤 1：创建 videoproxy.h**

从参考实现复制，适配命名空间。移除对 `WallpaperConfig` 的直接依赖（fillMode/showFps 改从 DConfig 读取）。

- [ ] **步骤 2：创建 videoproxy.cpp**

从参考实现复制（187行），适配：
- `setWindowFlags(Qt::Widget)` — 嵌入非独立窗口
- `WA_NoSystemBackground` + `WA_OpaquePaintEvent`
- `presentPixmap(QPixmap)` → `paintEvent` 用 `QPainter::drawPixmap`
- Fill/Fit/Stretch/Center/Tile 填充模式
- FPS 叠层（可选显示）

- [ ] **步骤 3：Commit**

```bash
git add src/desktop-edge/videoproxy.h src/desktop-edge/videoproxy.cpp
git commit -m "feat(edge): add VideoProxy embedded QWidget for desktop rendering"
```

---

## 任务 6：WallpaperEngine（核心引擎）

**文件：**
- 创建：`src/desktop-edge/engine.h`, `src/desktop-edge/engine_p.h`, `src/desktop-edge/engine.cpp`

**参考：** 从参考实现 `wallpaperengine.h/cpp/p.h` 复制并适配。

- [ ] **步骤 1：创建 engine.h**

适配命名空间，保留核心接口：`init()`, `turnOn()`, `turnOff()`, `refreshSource()`, `build()`, `play()`, `show()`, `geometryChanged()`。

- [ ] **步骤 2：创建 engine_p.h**

私有类，包含 `widgets`, `screenVideo`, `decoders`, `startDebounce` 等。

- [ ] **步骤 3：创建 engine.cpp**

从参考实现复制核心逻辑（~750行），适配：
- **配置源**：从 `MemeConfig`（DConfig 封装）读取，而非 JSON 文件
- **视频源**：`videoForScreen()` 扫描预置 + 用户目录
- **隐藏原生背景**：`setBackgroundVisibleFor(name, false)`
- **桌面框架信号**：CanvasCoreSubscribe
- **锁屏/屏保**：setupPowerHooks 暂停/恢复
- **FFmpeg 共享解码**：startSharedDecoders/stopSharedDecoders

- [ ] **步骤 4：Commit**

```bash
git add src/desktop-edge/engine.h src/desktop-edge/engine_p.h src/desktop-edge/engine.cpp
git commit -m "feat(edge): add WallpaperEngine core engine

Embeds into desktop framework, manages VideoProxy widgets per screen,
shared FFmpeg decoders, screen geometry, lock screen/suspension hooks."
```

---

## 任务 7：右键菜单场景

**文件：**
- 创建：`src/desktop-edge/menu.h`, `src/desktop-edge/menu.cpp`

**参考：** 从参考实现 `videowallpapermenuscene.h/cpp` 复制并适配。

- [ ] **步骤 1：创建 menu.h**

AbstractMenuScene 子类，注册到 CanvasMenu 场景。

- [ ] **步骤 2：创建 menu.cpp**

- "动态壁纸"(checkable 开关) → emit changeEnableState
- "动态壁纸设置…" → 通过 D-Bus 调用控制中心打开（`org.deepin.dde.ControlCenter` `/org/deepin/dde/ControlCenter` `ShowPage` 方法，page="personalization/meme"）

- [ ] **步骤 3：Commit**

```bash
git add src/desktop-edge/menu.h src/desktop-edge/menu.cpp
git commit -m "feat(edge): add desktop right-click menu for wallpaper toggle and settings"
```

---

## 任务 8：DConfig 读取器

**文件：**
- 创建：`src/desktop-edge/config.h`, `src/desktop-edge/config.cpp`

- [ ] **步骤 1：创建 config.h**

```cpp
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEME_EDGE_CONFIG_H
#define MEME_EDGE_CONFIG_H

#include <QObject>
#include <QString>

namespace DD_MEME_WALLPAPER_NAMESPACE {

enum class DecodeMode { Auto, Cuda, Vaapi, Software };
enum class FillMode { Fill, Fit, Stretch, Center, Tile };

class MemeConfig : public QObject
{
    Q_OBJECT
public:
    explicit MemeConfig(QObject *parent = nullptr);
    bool enabled() const;
    QString currentVideo() const;
    DecodeMode decodeMode() const;
    FillMode fillMode() const;
    void setEnabled(bool e);
    void setCurrentVideo(const QString &path);
signals:
    void configChanged();
};
}
#endif
```

- [ ] **步骤 2：创建 config.cpp**

使用 `DConfig::create("org.deepin.meme", "org.deepin.meme")` 读写 DConfig，监听 `valueChanged` 发出 `configChanged` 信号。

- [ ] **步骤 3：Commit**

```bash
git add src/desktop-edge/config.h src/desktop-edge/config.cpp
git commit -m "feat(edge): add DConfig reader for shared configuration"
```

---

## 任务 9：构建验证 edge 插件

- [ ] **步骤 1：构建**

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr 2>&1 | tail -10
cmake --build build -j$(nproc) 2>&1 | tail -20
```
预期：编译通过，生成 `libdd-meme-wallpaper-plugin.so`

- [ ] **步骤 2：安装 edge 插件**

```bash
echo "liu920618@" | sudo -S cmake --install build 2>&1 | grep -E "edge|desktop"
```

- [ ] **步骤 3：重启桌面验证插件加载**

```bash
systemctl --user restart 'dde-shell-plugin@org.deepin.ds.desktop.service'
sleep 2
journalctl --user -u 'dde-shell-plugin@org.deepin.ds.desktop.service' --no-pager -n 20 2>&1 | grep -iE "meme|wallpaper|error"
```
预期：日志中可见插件加载，无错误

- [ ] **步骤 4：Commit（如有修复）**

```bash
git add -A
git commit -m "fix(edge): resolve build issues for edge plugin"
```

---

## 任务 10：控制中心插件 — 壁纸列表模型

**文件：**
- 创建：`src/control-center-plugin/src/model.h`, `src/control-center-plugin/src/model.cpp`
- 修改：`src/control-center-plugin/src/memeplugin.h`, `src/control-center-plugin/src/memeplugin.cpp`

- [ ] **步骤 1：创建 model.h**

```cpp
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEME_WALLPAPER_MODEL_H
#define MEME_WALLPAPER_MODEL_H

#include <QAbstractListModel>
#include <QStringList>

struct WallpaperEntry {
    QString name;    // 文件名
    QString path;    // 绝对路径
    QString thumb;   // 缩略图路径（ffprobe 首帧）
    bool isPreset;   // true=预置, false=用户上传
};

class WallpaperModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Roles { NameRole = Qt::UserRole + 1, PathRole, ThumbRole, IsPresetRole };
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const;
    void refresh();  // 重新扫描预置 + 用户目录
    Q_INVOKABLE void removeUserWallpaper(int index);
    Q_INVOKABLE QString pathAt(int index) const;
signals:
    void countChanged();
private:
    QList<WallpaperEntry> m_wallpapers;
    void scanPreset();
    void scanUser();
};
#endif
```

- [ ] **步骤 2：创建 model.cpp**

- `scanPreset()`: 扫描 `/usr/share/deepin-meme-wallpapers/*.mp4`
- `scanUser()`: 扫描 `~/.local/share/deepin-meme-wallpapers/*.mp4`
- `refresh()`: 清空 + 两个目录合并
- `removeUserWallpaper()`: 删除用户目录中的文件 + refresh

- [ ] **步骤 3：Commit**

```bash
git add src/control-center-plugin/src/model.h src/control-center-plugin/src/model.cpp
git commit -m "feat(dcc): add WallpaperModel for preset + user video list"
```

---

## 任务 11：控制中心插件 — 视频上传与格式转换

**文件：**
- 创建：`src/control-center-plugin/src/converter.h`, `src/control-center-plugin/src/converter.cpp`

- [ ] **步骤 1：创建 converter.h**

```cpp
// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEME_VIDEO_CONVERTER_H
#define MEME_VIDEO_CONVERTER_H

#include <QObject>
#include <QString>

class VideoConverter : public QObject
{
    Q_OBJECT
public:
    explicit VideoConverter(QObject *parent = nullptr);
    // 检查视频是否为 H264/MP4，返回 true 表示符合
    static bool checkFormat(const QString &path);
    // 转换为 H264 MP4，异步，发出 progress 和 finished 信号
    void convert(const QString &inputPath, const QString &outputDir);
    void cancel();
signals:
    void progress(int percent);
    void finished(bool success, const QString &outputPath, const QString &error);
private:
    class QProcess *m_process = nullptr;
    QString generateOutputPath(const QString &inputPath, const QString &outputDir) const;
};
#endif
```

- [ ] **步骤 2：创建 converter.cpp**

`checkFormat()`:
```cpp
bool VideoConverter::checkFormat(const QString &path)
{
    QProcess proc;
    proc.start("ffprobe", {"-v", "error", "-select_streams", "v:0",
                "-show_entries", "stream=codec_name",
                "-of", "default=noprint_wrappers=1:nokey=1", path});
    proc.waitForFinished(5000);
    QString codec = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    return codec == "h264";
}
```

`convert()`:
```cpp
void VideoConverter::convert(const QString &inputPath, const QString &outputDir)
{
    QString outPath = generateOutputPath(inputPath, outputDir);
    m_process = new QProcess(this);
    QStringList args = {"-y", "-i", inputPath,
        "-c:v", "libx264", "-preset", "fast", "-crf", "23",
        "-c:a", "aac", "-movflags", "+faststart",
        "-progress", "pipe:1", "-nostats", outPath};
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        // 解析 "frame=xxx" 计算进度
    });
    connect(m_process, QOverload<int>::of(&QProcess::finished), this,
        [this, outPath](int code) {
        emit finished(code == 0, code == 0 ? outPath : "", m_process->readAllStandardError());
        m_process->deleteLater();
    });
    m_process->start("ffmpeg", args);
}
```

- [ ] **步骤 3：Commit**

```bash
git add src/control-center-plugin/src/converter.h src/control-center-plugin/src/converter.cpp
git commit -m "feat(dcc): add VideoConverter with ffprobe check + ffmpeg transcode"
```

---

## 任务 12：控制中心插件 — 重写 plugin 和 QML

**文件：**
- 修改：`src/control-center-plugin/src/memeplugin.h`, `src/control-center-plugin/src/memeplugin.cpp`
- 修改：`src/control-center-plugin/MemeMain.qml`
- 修改：`src/control-center-plugin/CMakeLists.txt`

- [ ] **步骤 1：重写 memeplugin.h/cpp**

- 移除旧的 `applyWallpaper()` D-Bus 调用
- 添加 `WallpaperModel *m_model` 和 `VideoConverter *m_converter`
- `Q_PROPERTY` 暴露 model、enabled、currentVideo、converting、convertProgress
- `applyWallpaper(path)`: 写 DConfig `currentVideo` + `enabled=true`
- `setEnabled(bool)`: 写 DConfig
- `uploadVideo(url)`: 调 `VideoConverter::checkFormat` → 直接复制或转码
- `removeUserWallpaper(int index)`: 调 model 删除
- 监听 DConfig 变化同步 UI

- [ ] **步骤 2：重写 MemeMain.qml**

```qml
import org.deepin.dcc 1.0
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

DccObject {
    id: root
    // ... (name, parentName, displayName, icon, weight)

    DccObject {
        name: "memeMain"
        parentName: "meme"
        weight: 10
        pageType: DccObject.Item
        pageSource: "MemeMain.qml"
    }
}
```

MemeMain.qml 核心 UI：
- 启用/禁用开关
- 壁纸网格 GridView（缩略图 + 名称 + 预置/上传标记）
- "上传视频"按钮
- 转码进度条（当 converting=true 时显示）
- "应用"按钮
- 删除按钮（仅用户上传显示）

- [ ] **步骤 3：更新 CMakeLists.txt**

移除 Qt6::Multimedia/Quick/Qml（如果不再用 QML Video 预览），添加 Qt6::Concurrent。

- [ ] **步骤 4：Commit**

```bash
git add src/control-center-plugin/
git commit -m "feat(dcc): rewrite control center plugin with model + converter + upload UI"
```

---

## 任务 13：Debian 打包

**文件：**
- 修改：`debian/control`, `debian/rules`, `debian/deepin-meme-plugin.install`
- 创建/修改：`debian/postinst`, `debian/prerm`

- [ ] **步骤 1：重写 debian/control**

```control
Source: deepin-meme-plugin
Section: utils
Priority: optional
Maintainer: UnionTech Software Technology Co., Ltd.
Build-Depends:
 debhelper-compat (= 13),
 cmake,
 pkg-config,
 qt6-base-dev,
 libdtk6core-dev,
 libdtk6config-dev,
 libavformat-dev,
 libavcodec-dev,
 libavutil-dev,
 libswscale-dev,
 libdde-file-manager,
 dde-control-center-dev
Standards-Version: 4.6.2
Rules-Requires-Root: no

Package: deepin-meme-plugin
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends},
 libdde-file-manager,
 ffmpeg,
 dde-control-center
Description: Deepin dynamic wallpaper plugin
 Desktop-edge plugin for dde-file-manager with FFmpeg video decoding,
 control center UI with video upload and format conversion.
```

- [ ] **步骤 2：重写 debian/rules**

```makefile
#!/usr/bin/make -f
export DH_VERBOSE = 1
%:
	dh $@ --buildsystem=cmake
override_dh_auto_configure:
	dh_auto_configure -- -DCMAKE_INSTALL_PREFIX=/usr
```

- [ ] **步骤 3：重写 deepin-meme-plugin.install**

```
usr/lib/*/dde-file-manager/plugins/desktop-edge/*.so
usr/lib/*/dde-control-center/plugins_v1.1/meme/*
usr/share/deepin-meme-wallpapers/*.mp4
usr/share/dsg/configs/org.deepin.meme/*.json
usr/share/deepin-meme-plugin/translations/*.qm
usr/share/icons/hicolor/scalable/apps/meme_icon.svg
```

- [ ] **步骤 4：重写 postinst**

```bash
#!/bin/sh
set -e
case "$1" in
    configure)
        for uid in $(ls /run/user 2>/dev/null || true); do
            case "$uid" in *[!0-9]*) continue ;; esac
            [ -S "/run/user/$uid/bus" ] || continue
            user=$(getent passwd "$uid" | cut -d: -f1)
            su -s /bin/sh -c "XDG_RUNTIME_DIR=/run/user/$uid \
                DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$uid/bus \
                timeout 8 systemctl --user restart \
                'dde-shell-plugin@org.deepin.ds.desktop.service'" "$user &
        done
        ;;
esac
#DEBHELPER#
exit 0
```

- [ ] **步骤 5：Commit**

```bash
git add debian/
git commit -m "debian: update packaging for edge plugin + control center architecture"
```

---

## 任务 14：构建、安装、端到端验证

- [ ] **步骤 1：完整构建**

```bash
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr 2>&1 | tail -10
cmake --build build -j$(nproc) 2>&1 | tail -20
```
预期：编译通过，生成 `libdd-meme-wallpaper-plugin.so` 和 `libmeme.so`

- [ ] **步骤 2：安装到系统**

```bash
echo "liu920618@" | sudo -S cmake --install build 2>&1 | tail -20
```

- [ ] **步骤 3：重启桌面验证 edge 插件加载**

```bash
systemctl --user restart 'dde-shell-plugin@org.deepin.ds.desktop.service'
sleep 3
journalctl --user -u 'dde-shell-plugin@org.deepin.ds.desktop.service' --no-pager -n 30 2>&1 | grep -iE "meme|wallpaper|error|fail"
```
预期：插件加载日志，无错误

- [ ] **步骤 4：通过 DConfig 启用验证视频壁纸**

```bash
# 通过 DConfig 设置启用 + 视频路径
/usr/bin/dconfig-cli set -d org.deepin.meme -k enabled -v true
/usr/bin/dconfig-cli set -d org.deepin.meme -k currentVideo -v "/usr/share/deepin-meme-wallpapers/【哲风壁纸】光之巨人-光线技能_1920x1080.mp4"
sleep 3
# 验证桌面有视频壁纸
journalctl --user -u 'dde-shell-plugin@org.deepin.ds.desktop.service' --no-pager -n 20 2>&1 | grep -iE "meme|frame|decode"
```

- [ ] **步骤 5：验证右键菜单**

```bash
# 通过 KWin 截图确认视频在桌面显示
SCREEN=$(qdbus org.kde.KWin /Screenshot org.kde.kwin.Screenshot.screenshotArea 0 0 1920 1080 2>&1)
python3 -c "
from PIL import Image
img = Image.open('$SCREEN').convert('RGB')
colors = img.getcolors(maxcolors=300000)
print(f'Unique colors: {len(colors)}')
# 视频在播放时颜色数应 > 10000
"
```

- [ ] **步骤 6：验证 debian 打包**

```bash
rm -f debian/debhelper-build-stamp
dpkg-buildpackage -us -uc -b 2>&1 | tail -15
```
预期：构建成功，生成 `.deb`

- [ ] **步骤 7：Commit**

```bash
git add -A
git commit -m "feat: complete video wallpaper refactor with edge plugin + upload

Three-component architecture: dde-file-manager desktop-edge plugin with
FFmpeg decoding + control center plugin with video upload/format conversion +
DConfig shared config. Verified: plugin loads, video renders on desktop,
right-click menu works, dpkg-buildpackage succeeds."
```

---

## 自检

### 规格覆盖度
- ✅ edge 插件嵌入桌面渲染 → 任务 3-6, 9
- ✅ FFmpeg 解码 → 任务 4
- ✅ 控制中心 UI → 任务 10-12
- ✅ 视频上传 + 格式转换 → 任务 11
- ✅ DConfig 共享配置 → 任务 2, 8
- ✅ 右键菜单 → 任务 7
- ✅ 预置 + 用户上传合并展示 → 任务 10
- ✅ Debian 打包 → 任务 13
- ✅ 构建验证 → 任务 9, 14

### 类型一致性
- `WallpaperEngine` — 任务 3 定义，任务 6 实现，任务 7/8/9 使用 ✅
- `VideoProxy` / `VideoProxyPointer` — 任务 5 定义，任务 6 使用 ✅
- `VideoDecoder` — 任务 4 定义，任务 6 使用 ✅
- `MemeConfig` — 任务 8 定义，任务 6/7 使用 ✅
- `WallpaperModel` — 任务 10 定义，任务 12 使用 ✅
- `VideoConverter` — 任务 11 定义，任务 12 使用 ✅
