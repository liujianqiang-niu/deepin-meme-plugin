// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef WALLPAPERMANAGER_H
#define WALLPAPERMANAGER_H

#include <QObject>
#include <QString>

namespace meme {
class ThemeResolver;
}

// 静态壁纸管理器: 通过 Appearance1 D-Bus 设置桌面壁纸
// 使用系统级壁纸设置机制,不会创建额外窗口,不覆盖桌面图标。
class WallpaperManager : public QObject
{
    Q_OBJECT
public:
    explicit WallpaperManager(meme::ThemeResolver *resolver, QObject *parent = nullptr);
    ~WallpaperManager();

    void enable();
    void disable();
    void setTheme(const QString &themeId);

    bool isActive() const { return m_active; }

private:
    bool setWallpaperViaDBus(const QString &imagePath);

private:
    meme::ThemeResolver *m_resolver;
    QString m_currentTheme;
    bool m_active = false;
    QString m_previousWallpaper;
};

#endif // WALLPAPERMANAGER_H
