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
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusInterface>
#include <QDBusReply>
#include <QFile>
#include <QProcessEnvironment>
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

QString WallpaperManager::sessionType() const
{
    // 1. 优先用环境变量 XDG_SESSION_TYPE
    const QString env = QProcessEnvironment::systemEnvironment().value(
        QStringLiteral("XDG_SESSION_TYPE"));
    if (!env.isEmpty()) return env;

    // 2. 检测 WAYLAND_DISPLAY
    if (!QProcessEnvironment::systemEnvironment().value(QStringLiteral("WAYLAND_DISPLAY")).isEmpty()) {
        return QStringLiteral("wayland");
    }
    return QStringLiteral("x11");
}

void WallpaperManager::setTheme(const QString &themeId)
{
    m_currentTheme = themeId;
    if (m_active) {
        // 主题切换时重新应用壁纸
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

    qCInfo(memeWallpaper) << "Enabling dynamic wallpaper, theme:" << m_currentTheme
                          << "session:" << sessionType();

    // 优先尝试 Wayland D-Bus 方式
    if (sessionType() == QStringLiteral("wayland")) {
        if (setVideoWallpaperViaDBus(videoPath)) {
            m_active = true;
            return;
        }
        qCWarning(memeWallpaper) << "Wayland D-Bus wallpaper set failed, fallback to window";
    }

    // X11 或 Wayland D-Bus 失败时,用贴底层窗口
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

    // Wayland: 通过 D-Bus 恢复默认壁纸(设置一个空字符串或调用 reset)
    if (sessionType() == QStringLiteral("wayland")) {
        auto bus = QDBusConnection::sessionBus();
        // 这里不主动改回静态壁纸,避免覆盖用户原壁纸
        // 用户需自行在控制中心恢复壁纸设置
    }

    m_active = false;
    qCInfo(memeWallpaper) << "Dynamic wallpaper disabled";
}

bool WallpaperManager::setVideoWallpaperViaDBus(const QString &videoPath)
{
    // 通过 org.deepin.dde.Appearance1 设置壁纸
    // Wayland 下 Appearance1 支持 type=video 的壁纸
    QDBusInterface iface("org.deepin.dde.Appearance1",
                         "/org/deepin/dde/Appearance1",
                         "org.deepin.dde.Appearance1",
                         QDBusConnection::sessionBus());
    if (!iface.isValid()) {
        qCWarning(memeWallpaper) << "Appearance1 D-Bus unavailable";
        return false;
    }

    // 调用 SetWallpaper(monitorName, wallpaperPath, type)
    // type: "image" 或 "video"
    QDBusReply<void> reply = iface.call("SetWallpaper",
        QStringLiteral("eDP-1"),  // MVP: 用主屏名,后续遍历
        videoPath,
        QStringLiteral("video"));
    if (!reply.isValid()) {
        qCWarning(memeWallpaper) << "SetWallpaper failed:" << reply.error().message();
        return false;
    }
    qCInfo(memeWallpaper) << "Wallpaper set via D-Bus:" << videoPath;
    return true;
}

bool WallpaperManager::createVideoWallpaperWindow(const QString &videoPath)
{
    // X11 fallback: 创建贴底层无边框窗口循环播放视频
    if (!m_wallpaperWindow) {
        m_wallpaperWindow = new QWidget();
        m_wallpaperWindow->setWindowFlags(
            Qt::FramelessWindowHint |
            Qt::WindowStaysOnBottomHint |
            Qt::Tool
        );
        m_wallpaperWindow->setAttribute(Qt::WA_TranslucentBackground);
        m_wallpaperWindow->setAttribute(Qt::WA_ShowWithoutActivating);

        m_wallpaperWidget = new QVideoWidget(m_wallpaperWindow);
        m_wallpaperPlayer = new QMediaPlayer(this);
        m_wallpaperPlayer->setVideoOutput(m_wallpaperWidget);
        m_wallpaperPlayer->setAudioOutput(nullptr);  // 壁纸静音
    }

    // 全屏覆盖到主屏(多显示器后续扩展)
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return false;
    m_wallpaperWindow->setGeometry(screen->geometry());
    m_wallpaperWidget->setGeometry(0, 0, screen->geometry().width(), screen->geometry().height());

    m_wallpaperPlayer->setSource(QUrl::fromLocalFile(videoPath));
    // 循环播放
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
