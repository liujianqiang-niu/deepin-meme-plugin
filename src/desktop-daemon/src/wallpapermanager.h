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
// 实现策略: 创建 Qt::Desktop 类型窗口(_NET_WM_WINDOW_TYPE_DESKTOP),
//           位于桌面图标层(CanvasView)之下,不覆盖图标。
class WallpaperManager : public QObject
{
    Q_OBJECT
public:
    explicit WallpaperManager(meme::ThemeResolver *resolver, QObject *parent = nullptr);
    ~WallpaperManager();

    // 启用动态壁纸(应用当前主题的 wallpaper.mp4)
    void enable();

    // 禁用动态壁纸
    void disable();

    // 切换主题时重新应用壁纸
    void setTheme(const QString &themeId);

    bool isActive() const { return m_active; }

private:
    // 创建 Qt::Desktop 类型视频窗口(位于桌面图标层之下)
    bool createVideoWallpaperWindow(const QString &videoPath);

private:
    meme::ThemeResolver *m_resolver;
    QString m_currentTheme;
    bool m_active = false;

    QMediaPlayer *m_wallpaperPlayer = nullptr;
    QVideoWidget *m_wallpaperWidget = nullptr;
    QWidget *m_wallpaperWindow = nullptr;
};

#endif // WALLPAPERMANAGER_H
