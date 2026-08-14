// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "wallpaperplayer.h"
#include "memedconfig.h"

#include <QGuiApplication>
#include <QScreen>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QWidget>
#include <QFile>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(memeWallpaper, "meme.wallpaper")

WallpaperPlayer::WallpaperPlayer(QObject *parent)
    : QObject(parent)
{
    // Qt::Desktop 映射到 _NET_WM_WINDOW_TYPE_DESKTOP,
    // kwin 将其放在桌面图标层之下,不会遮挡图标。
    m_window = new QWidget();
    m_window->setWindowFlags(Qt::Desktop);
    m_window->setAttribute(Qt::WA_TranslucentBackground);

    m_videoWidget = new QVideoWidget(m_window);
    m_player = new QMediaPlayer(this);
    m_player->setVideoOutput(m_videoWidget);
    m_player->setAudioOutput(nullptr);

    connect(m_player, &QMediaPlayer::playbackStateChanged, this,
        [this](QMediaPlayer::PlaybackState state) {
            if (state == QMediaPlayer::StoppedState && m_player->source().isValid()) {
                m_player->play();
            }
        });
}

WallpaperPlayer::~WallpaperPlayer()
{
    if (m_player) m_player->stop();
    delete m_window;
}

void WallpaperPlayer::setVideo(const QString &path)
{
    if (path.isEmpty() || !QFile::exists(path)) {
        qCWarning(memeWallpaper) << "Video not found:" << path;
        return;
    }

    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    m_window->setGeometry(screen->geometry());
    m_videoWidget->setGeometry(0, 0, screen->geometry().width(), screen->geometry().height());

    m_player->setSource(QUrl::fromLocalFile(path));
    m_window->show();
    m_player->play();
    qCInfo(memeWallpaper) << "Playing video wallpaper:" << path;
}

void WallpaperPlayer::stop()
{
    m_player->stop();
    m_window->hide();
}
