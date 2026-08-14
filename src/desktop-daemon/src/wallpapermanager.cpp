// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "wallpapermanager.h"
#include "themeresolver.h"
#include "thememanifest.h"

#include <QGuiApplication>
#include <QScreen>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QWidget>
#include <QFile>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(memeWallpaper, "meme.wallpaper")

WallpaperManager::WallpaperManager(meme::ThemeResolver *resolver, QObject *parent)
    : QObject(parent)
    , m_resolver(resolver)
{
}

WallpaperManager::~WallpaperManager()
{
    if (m_wallpaperWindow) {
        m_wallpaperWindow->hide();
        delete m_wallpaperWindow;
    }
}

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
    const QString videoPath = meme::resolvePath(themeDir, manifest->wallpaperPath);

    if (videoPath.isEmpty() || !QFile::exists(videoPath)) {
        qCWarning(memeWallpaper) << "Wallpaper video not found:" << videoPath;
        return;
    }

    qCInfo(memeWallpaper) << "Enabling dynamic wallpaper, theme:" << m_currentTheme;

    if (createVideoWallpaperWindow(videoPath)) {
        m_active = true;
    }
}

void WallpaperManager::disable()
{
    if (!m_active) return;

    if (m_wallpaperWindow) {
        m_wallpaperWindow->hide();
        if (m_wallpaperPlayer) m_wallpaperPlayer->stop();
    }

    m_active = false;
    qCInfo(memeWallpaper) << "Dynamic wallpaper disabled";
}

bool WallpaperManager::createVideoWallpaperWindow(const QString &videoPath)
{
    // Qt::Desktop 窗口类型映射到 _NET_WM_WINDOW_TYPE_DESKTOP,
    // 位于桌面图标层(CanvasView)之下,不会覆盖图标。
    if (!m_wallpaperWindow) {
        m_wallpaperWindow = new QWidget();
        m_wallpaperWindow->setWindowFlags(Qt::Desktop);
        m_wallpaperWindow->setAttribute(Qt::WA_TranslucentBackground);
        m_wallpaperWindow->setAttribute(Qt::WA_ShowWithoutActivating);

        m_wallpaperWidget = new QVideoWidget(m_wallpaperWindow);
        m_wallpaperPlayer = new QMediaPlayer(this);
        m_wallpaperPlayer->setVideoOutput(m_wallpaperWidget);
        m_wallpaperPlayer->setAudioOutput(nullptr);
    }

    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return false;
    m_wallpaperWindow->setGeometry(screen->geometry());
    m_wallpaperWidget->setGeometry(0, 0, screen->geometry().width(), screen->geometry().height());

    m_wallpaperPlayer->setSource(QUrl::fromLocalFile(videoPath));
    connect(m_wallpaperPlayer, &QMediaPlayer::playbackStateChanged, this,
        [this](QMediaPlayer::PlaybackState state) {
            if (state == QMediaPlayer::StoppedState && m_active) {
                m_wallpaperPlayer->play();
            }
        }, Qt::UniqueConnection);

    m_wallpaperWindow->show();
    m_wallpaperPlayer->play();
    qCInfo(memeWallpaper) << "Video wallpaper window created:" << videoPath;
    return true;
}
