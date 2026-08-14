// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef WALLPAPERMANAGER_H
#define WALLPAPERMANAGER_H

#include <QObject>
#include <QString>

namespace meme {
class ThemeResolver;
}

class QMediaPlayer;
class QVideoWidget;
class QWidget;

// 动态壁纸管理器: 在桌面背景层循环播放视频壁纸
// 实现策略:
//   1. Wayland/Treeland: 通过 org.deepin.dde.Appearance1 D-Bus 设置视频壁纸
//   2. X11: 守护进程自身创建一个贴底层的透明窗口循环播放视频
//          (Layer 5.0 同级,在 BackgroundDefault 之上或替换)
class WallpaperManager : public QObject
{
    Q_OBJECT
public:
    explicit WallpaperManager(meme::ThemeResolver *resolver, QObject *parent = nullptr);
    ~WallpaperManager();

    // 启用动态壁纸(应用当前主题的 wallpaper.mp4)
    void enable();

    // 禁用动态壁纸(恢复静态壁纸,需要外部记录原壁纸)
    void disable();

    // 切换主题时重新应用壁纸
    void setTheme(const QString &themeId);

    bool isActive() const { return m_active; }

private:
    // Wayland: 通过 D-Bus 设置视频壁纸
    bool setVideoWallpaperViaDBus(const QString &videoPath);

    // X11: 创建贴底层视频窗口
    bool createVideoWallpaperWindow(const QString &videoPath);

    // 检测当前会话类型(Wayland/X11)
    QString sessionType() const;

private:
    meme::ThemeResolver *m_resolver;
    QString m_currentTheme;
    bool m_active = false;

    // X11 fallback 视频窗口
    QMediaPlayer *m_wallpaperPlayer = nullptr;
    QVideoWidget *m_wallpaperWidget = nullptr;
    QWidget *m_wallpaperWindow = nullptr;
};

#endif // WALLPAPERMANAGER_H
