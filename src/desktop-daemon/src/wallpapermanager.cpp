// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "wallpapermanager.h"
#include "themeresolver.h"
#include "thememanifest.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QFile>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(memeWallpaper, "meme.wallpaper")

WallpaperManager::WallpaperManager(meme::ThemeResolver *resolver, QObject *parent)
    : QObject(parent)
    , m_resolver(resolver)
{
}

WallpaperManager::~WallpaperManager() = default;

void WallpaperManager::setTheme(const QString &themeId)
{
    m_currentTheme = themeId;
    if (m_active) {
        disable();
        enable();
    }
}

void WallpaperManager::enable()
{
    if (m_active) return;
    if (m_currentTheme.isEmpty()) {
        qCWarning(memeWallpaper) << "No theme set, cannot enable wallpaper";
        return;
    }

    auto manifest = m_resolver->manifest(m_currentTheme);
    if (!manifest) {
        qCWarning(memeWallpaper) << "Theme not found:" << m_currentTheme;
        return;
    }

    const QString themeDir = m_resolver->themeDirectory(m_currentTheme);
    const QString imagePath = meme::resolvePath(themeDir, manifest->wallpaperPath);

    if (imagePath.isEmpty() || !QFile::exists(imagePath)) {
        qCWarning(memeWallpaper) << "Wallpaper image not found:" << imagePath;
        return;
    }

    // 保存当前壁纸,以便禁用时恢复
    QDBusInterface iface("org.deepin.dde.Appearance1",
                         "/org/deepin/dde/Appearance1",
                         "org.deepin.dde.Appearance1",
                         QDBusConnection::sessionBus());
    if (iface.isValid()) {
        QDBusReply<QString> bgReply = iface.call("GetCurrentWorkspaceBackground");
        if (bgReply.isValid()) {
            m_previousWallpaper = bgReply.value();
        }
    }

    if (setWallpaperViaDBus(imagePath)) {
        m_active = true;
        qCInfo(memeWallpaper) << "Static wallpaper set:" << imagePath;
    }
}

void WallpaperManager::disable()
{
    if (!m_active) return;

    if (!m_previousWallpaper.isEmpty()) {
        setWallpaperViaDBus(m_previousWallpaper);
    }

    m_active = false;
    qCInfo(memeWallpaper) << "Wallpaper restored to:" << m_previousWallpaper;
}

bool WallpaperManager::setWallpaperViaDBus(const QString &imagePath)
{
    QDBusInterface iface("org.deepin.dde.Appearance1",
                         "/org/deepin/dde/Appearance1",
                         "org.deepin.dde.Appearance1",
                         QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qCWarning(memeWallpaper) << "Appearance1 D-Bus unavailable";
        return false;
    }

    const QString url = "file://" + imagePath;

    QDBusReply<void> reply = iface.call("SetCurrentWorkspaceBackground", url);
    if (!reply.isValid()) {
        qCWarning(memeWallpaper) << "SetCurrentWorkspaceBackground failed:" << reply.error().message();
        return false;
    }
    return true;
}
