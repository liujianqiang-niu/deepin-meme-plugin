# deepin-meme-plugin 架构设计

> 本文档描述 deepin-meme-plugin 3.1.0 的整体架构、数据流与关键实现细节，供二次开发与问题排查参考。

## 1. 总体架构

项目由两个独立的进程内插件组成，二者**不跨进程通信**，通过 DConfig（`org.deepin.meme`）共享状态：

| 插件 | 宿主进程 | 职责 |
|---|---|---|
| `desktop-edge`（`libdd-meme-wallpaper-plugin`） | dde-file-manager 桌面（dde-shell desktop service） | 视频解码 + 桌面渲染 + 右键菜单 |
| `control-center`（`meme.so` + `libmeme_qml.so`） | dde-control-center | 管理界面：开关 / 上传 / 预览 / 应用 / 删除 |

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

**配置流**：控制中心写入 `enabled` / `currentVideo` → DConfig 文件持久化 → edge 插件监听配置变化（`onOptionsChanged()`）→ 切换/停止壁纸。

## 2. 边缘插件（desktop-edge）

### 2.1 模块划分

| 文件 | 职责 |
|---|---|
| `plugin.cpp/h` | dpf 插件入口，`Q_PLUGIN_METADATA` 指向 `meme_videowallpaper.json`（Name: `dd-meme-wallpaper`，Depends: `ddplugin-core >= 1.0.0`） |
| `engine.cpp/h`、`engine_p.h` | 壁纸引擎：初始化、开关、资源检查、锁屏/屏保/可见性联动、多屏管理 |
| `decoder.cpp/h` | FFmpeg 解码线程（`QThread`），软解 / VAAPI / CUDA 三级回退 |
| `videoproxy.cpp/h` | 嵌入桌面 root 的 `QWidget`，绘制 + FPS 叠加 + 限速 |
| `videoframe.h` | 解码帧结构：`Rgb32`（软解）或 `Nv12`（硬解）两种格式 |
| `menu.cpp/h` | 桌面右键菜单场景（`AbstractMenuScene`） |
| `config.cpp/h` | DConfig 读取器（`MemeConfig`） |

### 2.2 生命周期

```
dpf 加载
  └─ initialize()          # 注册菜单场景（MenuSceneCreator）
  └─ start()
       ├─ checkResouce()   # 周期检查资源目录（QTimer 兜底）
       ├─ build()          # 遍历 desktopFrameRootWindows，为每屏创建 VideoProxy
       ├─ setupPowerHooks()# 锁屏 / 屏保 / 可见性 / 应用状态监听
       └─ refreshSource()  # 读 DConfig → 起解码线程
```

- 开关：`turnOn()` 构建小部件并启动解码；`turnOff()` 停解码、清帧。
- 配置：`onOptionsChanged()` 监听 DConfig 变化，`enabled` 关闭时调用 `turnOff()` 恢复静态壁纸。
- 暂停：解码线程与绘制统一管理——保活叠层（冻结帧）+ 资源目录存在性检查。

### 2.3 解码链路（decoder.cpp）

```
file → avformat_open_input → av_find_best_stream(video)
     → 硬解探测（createHwDevice：CUDA → VAAPI 依 DConfig 顺序，失败回退软解）
     → avcodec_open2 → 读包 avcodec_send_packet → avcodec_receive_frame
     → 硬解帧：hwdownload → NV12（无 sws）；软解帧：sws_scale → RGB32
     → 帧率节拍：nextPtsUs += frameIntervalUs，落后则 usleep，超前超 2 帧丢弃
     → emit frameReady(VideoFrame)
```

**关键参数**：

- 帧率：`opt.fps == 0.0` 表示跟随源帧率（`avg_frame_rate`）；配置 1~240 FPS 上限。
- 帧节拍：`frameIntervalUs = 1000000 / (targetFps * speed)`；`dropSlackUs = 1.5 × frameIntervalUs`。
- 背压：`inFlight`（最大 2）控制队列积压，超限则睡眠等待主线程消费。
- 出图宽度：`maxWidth` 取所有屏物理宽度最大值（逻辑宽 × DPR），保证 4K 屏清晰。

### 2.4 渲染链路（videoproxy.cpp）

- `VideoProxy` 直接 parent 到桌面 root widget（禁止独立顶层窗），几何 = root 相对几何。
- 呈现：`presentPixmap()` 主线程将 QImage 转成 QPixmap **一次**，多屏共享同一 QPixmap。
- 铺屏方式（`FillMode`）：`fill` 铺满裁切（默认）/ `fit` 自适应 / `stretch` 拉伸 / `center` 居中 / `tile` 平铺。
- FPS 叠加：`setShowFps(true)` 显示 `displayFps`（窗内计数）。
- 限速：`fpsCap == 0.0` 时跟随解码线程节拍；>0 时以 `minGapUs = 1e6 / fpsCap` 节流绘制。

### 2.5 引擎联动（engine.cpp）

| 事件 | 行为 |
|---|---|
| 锁屏 `Locked` / 屏保 `ActiveChanged(true)` | `setPlaybackSuspended(true)` → 停解码、清帧；解锁恢复 |
| 所有控件不可见（被全屏窗口遮挡） | 2 s 可见性定时器 → 挂起解码；恢复可见时恢复 |
| 应用 `ApplicationSuspended` | 挂起解码；`ApplicationActive` 且可见时恢复 |
| 屏幕几何变化 / 资源变更 | `geometryChanged()` / `refreshSource()` 重建 |

**静态壁纸回退**：`enabled=false` 或 `currentVideo` 无效时，`onOptionsChanged()` 走 `turnOff()` 分支，桌面恢复系统静态壁纸。

## 3. 控制中心插件（control-center）

### 3.1 模块划分

| 文件 | 职责 |
|---|---|
| `Meme.qml` | 模块根 DccObject（name: `meme`，parentName: `personalization`，weight: 350） |
| `MemeMain.qml` | 管理页：启用开关 / 上传进度 / 预览播放器 / 壁纸网格 |
| `memeplugin.cpp/h` | `MemePlugin`：桥接 QML 与 C++，Q_PROPERTY 暴露 `enabled`/`converting`/`convertProgress`/`statusMessage` 等 |
| `model.cpp/h` | `WallpaperModel`（QAbstractListModel）：预置 + 用户上传壁纸列表 |
| `converter.cpp/h` | `VideoConverter`：H264 检测 + ffmpeg 异步转码 + 进度计算 |

### 3.2 数据流

```
MemeMain.qml (DccObject page)
  ├─ Switch ──→ MemePlugin::setEnabled ──→ DConfig(enabled)
  ├─ FileDialog ──→ MemePlugin::uploadVideo(url)
  │                  └─ VideoConverter::checkFormat
  │                       ├─ H264 → 直接入库（无需转码）
  │                       └─ 其他 → VideoConverter::convert（QProcess ffmpeg）
  │                            ├─ progress(int) ──→ convertProgress ──→ ProgressBar
  │                            └─ finished(bool, path) ──→ 刷新模型 + statusMessage
  ├─ GridView delegate「预览」 ──→ urlFromPath + Video.play
  ├─ 「应用」 ──→ MemePlugin::applyWallpaper ──→ setCurrentVideo ──→ DConfig(currentVideo)
  └─ 「删除」 ──→ Model::removeUserWallpaper（仅非预置）
```

### 3.3 转码器（converter.cpp）

- 格式检测：`checkFormat()` 用 `ffprobe`（JSON 输出）检查 `codec_name == h264` 且容器为 MP4。
- 转码命令：`ffmpeg -i <in> -c:v libx264 -crf 23 -preset medium -c:a aac <out>`（进度通过 `-progress pipe:1` 解析）。
- 进度计算：`queryTotalFrames()` 两级回退——`nb_read_packets`（快）→ `duration × fps`（慢）；**失败时删除残缺输出文件**。
- 转码线程：`QProcess` 异步执行，不阻塞 UI；`cancel()` 发送终止信号。

### 3.4 壁纸模型（model.cpp）

- 预置目录：`/usr/share/deepin-meme-wallpapers/`（`isPreset = true`）。
- 用户目录：`~/.local/share/deepin-meme-wallpapers/`（`isPreset = false`，可删除）。
- `refresh()` 重新扫描两个目录；`pathAt()` / `removeUserWallpaper()` 供 QML 调用。

## 4. 配置（DConfig）

schema：`data/configs/org.deepin.meme.json`，应用 ID = `org.deepin.meme`。

| 键 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `enabled` | bool | `false` | 启用动态壁纸 |
| `currentVideo` | string | `""` | 当前壁纸绝对路径 |
| `decodeMode` | string | `"software"` | `software` / `cuda` / `vaapi` / `auto` |
| `fillMode` | string | `"fill"` | `fill` / `fit` / `stretch` / `center` / `tile` |

- 读写封装：`MemeConfig`（edge 侧）与 `MemePlugin` 内联 DConfig（控制中心侧），两侧共用同一 schema。
- `decodeMode` / `fillMode` 字符串 ↔ 枚举转换集中在 `config.cpp` 的静态方法。

## 5. 打包与安装

- 构建：CMake（`CMAKE_CXX_STANDARD 17`），依赖 Qt6 ≥ 6.8、Dtk6 ≥ 6.7、FFmpeg、`dfm6-base` / `dfm6-framework`（边缘插件）、`libdde-control-center`（控制中心插件）。
- 打包：`dpkg-buildpackage -us -uc -b`，产出 `deepin-meme-plugin`（两个 `.so`）与 `deepin-meme-plugin-data`（预置视频 + schema）。
- `postinst`：安装后自动 `systemctl --user restart dde-shell-plugin@org.deepin.ds.desktop.service` 重新加载边缘插件。
- 安装路径：见 `README.md` 「安装产物路径」章节。

> **已知坑**：`dh_install` 可能在打包时带入旧构建的边缘插件 `.so`（staging 中的旧文件覆盖新文件），重新打包前务必清理 `debian/tmp`、`debian/deepin-meme-plugin/` 等中间产物。

## 6. 关键决策记录

1. **嵌入渲染而非独立窗口**：`VideoProxy` 直接 parent 到桌面 root，避免 Wayland 下独立窗口被合成器/焦点策略干扰，且天然跟随桌面几何。
2. **QPixmap 共享**：多屏只 `fromImage()` 一次，节省 GPU/内存带宽。
3. **DConfig 而非 DBus**：早期方案（daemon + DBus）被废弃，改为纯插件 + 配置文件共享，减少系统服务依赖。
4. **NV12 直通**：硬解路径走 NV12 面（无 sws），软解路径走 RGB32，减少不必要的色彩空间转换。
5. **AV1 兼容策略**：壁纸引擎只保证 H264，其它编码（AV1/HEVC 等）由控制中心上传时自动转码兜底。