# 动态壁纸重构设计：dde-file-manager edge 插件 + 控制中心 + DConfig

> 日期: 2026-08-15
> 状态: 已确认

## 1. 背景与目标

### 1.1 问题
当前 deepin-meme-plugin 使用独立 daemon 进程 + 独立顶层窗口播放视频。此方案存在根本性架构问题：
- 视频窗口作为普通应用窗口出现，带标题栏和任务栏条目
- QQuickView/QVideoWidget 在 systemd 服务环境中渲染不可靠
- 需要设置透明 PNG 壁纸 hack，不够干净

### 1.2 目标
重构为 dde-file-manager desktop-edge 插件架构（参考 `deepin_video_wallpaper` 实现），实现：
1. 视频壁纸嵌入桌面框架，行为像原生壁纸
2. 控制中心插件提供 UI 管理界面
3. 支持用户上传视频，自动检查/转换格式
4. 保留桌面右键菜单
5. 预置视频 + 用户上传合并展示

### 1.3 确认的设计约束
- 通信机制: DConfig 共享配置
- 用户上传存储: `~/.local/share/deepin-meme-wallpapers/`（/usr/share 只读）
- 格式转换工具: ffmpeg CLI
- 视频展示: 预置 + 用户上传合并
- 右键菜单: 保留 + 控制中心

## 2. 架构

### 2.1 三组件架构

| 组件 | 类型 | 安装位置 | 职责 |
|------|------|----------|------|
| `dd-meme-wallpaper` | dde-file-manager edge 插件 | `lib/*/dde-file-manager/plugins/desktop-edge/` | FFmpeg 解码、嵌入桌面渲染、右键菜单 |
| `meme` | dde-control-center 插件 | `lib/*/dde-control-center/plugins_v1.1/meme/` | UI: 预览、选择、上传、格式转换 |
| `org.deepin.meme` | DConfig schema | `/usr/share/dsg/configs/` | 共享配置 |

### 2.2 通信流

```
控制中心插件 → 写 DConfig → DConfig::valueChanged(跨进程) → edge 插件监听 → 刷新视频
桌面右键菜单 → 直接操作 WallpaperEngine(同进程) → 写 DConfig → 控制中心同步
```

### 2.3 视频目录

| 目录 | 用途 | 权限 |
|------|------|------|
| `/usr/share/deepin-meme-wallpapers/` | 预置壁纸 | 只读 |
| `~/.local/share/deepin-meme-wallpapers/` | 用户上传 | 可写 |

## 3. 目录结构

```
deepin-meme-plugin/
├── CMakeLists.txt
├── data/
│   ├── configs/org.deepin.meme.json
│   ├── icons/meme_icon.svg
│   └── res/*.mp4 (5个预置视频)
├── src/
│   ├── desktop-edge/
│   │   ├── CMakeLists.txt
│   │   ├── meme_videowallpaper.json
│   │   ├── global.h
│   │   ├── plugin.cpp/h
│   │   ├── engine.cpp/h, engine_p.h
│   │   ├── videoproxy.cpp/h
│   │   ├── decoder.cpp/h
│   │   ├── videoframe.h
│   │   ├── menu.cpp/h
│   │   └── config.cpp/h
│   └── control-center-plugin/
│       ├── CMakeLists.txt
│       ├── Meme.qml, MemeMain.qml
│       └── src/
│           ├── plugin.cpp/h
│           ├── model.cpp/h
│           └── converter.cpp/h
├── debian/
│   ├── control, rules, postinst, prerm
│   └── deepin-meme-plugin.install
└── scripts/build-deb.sh
```

删除: `src/desktop-daemon/`, `src/common/`, `data/autostart/`, `data/systemd/`, `data/dbus/`

## 4. DConfig Schema

`data/configs/org.deepin.meme.json`:

| 键 | 类型 | 默认值 | 说明 |
|----|------|--------|------|
| `enabled` | bool | false | 启用动态壁纸 |
| `currentVideo` | string | "" | 当前视频绝对路径 |
| `decodeMode` | string | "software" | 解码模式: software/cuda/vaapi/auto |
| `fillMode` | string | "fill" | 铺屏: fill/fit/stretch/center/tile |

## 5. desktop-edge 插件

### 5.1 插件入口
- `Q_PLUGIN_METADATA(IID "org.deepin.plugin.desktop" FILE "meme_videowallpaper.json")`
- 依赖: `ddplugin-core`
- `start()` 创建 `WallpaperEngine`，`stop()` 销毁

### 5.2 WallpaperEngine
基于参考实现适配:
- 配置源: DConfig（监听 `DConfig::valueChanged`）
- 视频源: 扫描预置 + 用户两个目录
- 嵌入桌面: `VideoProxy(root)` 以 root 为 parent
- 隐藏原生背景: `setBackgroundVisibleFor(name, false)` 隐藏 `kPropWidgetName=="background"` 的 widget
- 桌面框架信号: 订阅 `signal_DesktopFrame_WindowBuilded/GeometryChanged/WindowAboutToBeBuilded`
- 锁屏/屏保: 暂停/恢复解码
- FFmpeg 共享解码: 同一视频多屏共享一个 decoder

### 5.3 VideoDecoder
QThread 子类:
- `avformat_open_input` → `avcodec_decode` → `sws_scale` 转 BGRA → `emit frameReady`
- 支持 CUDA/VAAPI/Software
- 帧率控制、丢帧、双缓冲、循环播放

### 5.4 VideoProxy
QWidget 嵌入桌面:
- `setWindowFlags(Qt::Widget)` 非独立窗口
- `WA_NoSystemBackground` + `WA_OpaquePaintEvent`
- `presentPixmap(QPixmap)` → `paintEvent` 用 `QPainter::drawPixmap`
- 多填充模式: Fill/Fit/Stretch/Center/Tile

### 5.5 右键菜单
- 注册到 `CanvasMenu` 场景
- "动态壁纸"(开关) + "动态壁纸设置…"(打开控制中心)

## 6. 控制中心插件

### 6.1 壁纸列表模型
- `QAbstractListModel` 合并预置 + 用户上传
- 每项: name, path, thumbnail(ffprobe 首帧), source(preset/user)
- 用户上传可删，预置不可删

### 6.2 上传与格式转换
1. 用户选视频文件
2. `ffprobe` 检查 codec(H264) + container(MP4)
3. 不符合则 `QProcess` 调 `ffmpeg`:
   ```
   ffmpeg -i input -c:v libx264 -preset fast -crf 23 \
     -c:a aac -movflags +faststart output.mp4
   ```
4. 进度通过 `QProcess::readyReadStandardOutput` 解析 `frame=` 更新 UI
5. 完成后刷新模型，写 DConfig

### 6.3 QML 界面
- 壁纸网格列表（缩略图预览）
- "上传视频"按钮 + 转码进度条
- "应用"按钮 → 写 DConfig
- 启用/禁用开关
- 删除用户视频

## 7. 构建依赖

### 系统依赖
- Qt6: Core Widgets Gui DBus Concurrent Network
- DTK6: Core
- FFmpeg: libavformat libavcodec libavutil libswscale
- dde-file-manager: libdfm6-base.so, libdfm6-framework.so, include 路径
- dde-control-center: include 路径

### CMake 配置
```cmake
set(DCC_SOURCE_ROOT ".../dde-control-center")
set(DFM_SOURCE_ROOT ".../dde-file-manager")
set(DFM_INCLUDE_DIR "${DFM_SOURCE_ROOT}/include")
find_library(DFM6_BASE_LIB dfm6-base REQUIRED)
find_library(DFM6_FRAMEWORK_LIB dfm6-framework REQUIRED)
```

## 8. 打包

- 包名: `deepin-meme-plugin`
- Depends: libdde-file-manager, FFmpeg libs, Qt6, dtk6
- postinst: 重启 `dde-shell-plugin@org.deepin.ds.desktop.service`
- 安装 edge 插件到 `lib/*/dde-file-manager/plugins/desktop-edge/`
- 安装控制中心插件到 `lib/*/dde-control-center/plugins_v1.1/meme/`
- 安装预置视频到 `/usr/share/deepin-meme-wallpapers/`
- 安装 DConfig schema 到 `/usr/share/dsg/configs/`

## 9. 验证标准

1. edge 插件加载到 dde-file-manager 进程，无独立进程
2. 视频嵌入桌面，不出现在任务栏
3. 桌面图标在视频上方可见
4. 控制中心可启用/禁用、选择壁纸
5. 控制中心可上传视频，非 H264/MP4 自动转码
6. 右键桌面有"动态壁纸"开关和"设置"入口
7. 预置 + 用户上传视频合并展示
8. `dpkg-buildpackage` 成功
