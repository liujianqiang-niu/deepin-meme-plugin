// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "wallpapermanager.h"
#include "themeresolver.h"
#include "thememanifest.h"

#include <QFile>
#include <QProcess>
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
    QProcess getter;
    getter.start("gsettings", {"get", "com.deepin.dde.appearance", "background-uris"});
    getter.waitForFinished(3000);
    QString output = QString::fromUtf8(getter.readAllStandardOutput()).trimmed();
    // gsettings 返回格式: "['file:///path/to/wallpaper.jpg']"
    // 提取第一个 URI
    int start = output.indexOf("'");
    int end = output.indexOf("'", start + 1);
    if (start >= 0 && end > start) {
        m_previousWallpaper = output.mid(start + 1, end - start - 1);
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
    // v25 系统上 Appearance1 D-Bus Set/SetCurrentWorkspaceBackground 不生效,
    // 实际壁纸由 dde-shell 读取 gsettings com.deepin.dde.appearance background-uris 渲染。
    const QString uri = "file://" + imagePath;
    const QString gvariantValue = "['" + uri + "']";

    int ret = QProcess::execute("gsettings", {
        "set", "com.deepin.dde.appearance", "background-uris", gvariantValue
    });

    if (ret != 0) {
        qCWarning(memeWallpaper) << "gsettings set background-uris failed, ret:" << ret;
        return false;
    }

    qCInfo(memeWallpaper) << "Wallpaper set via gsettings:" << uri;
    return true;
}
