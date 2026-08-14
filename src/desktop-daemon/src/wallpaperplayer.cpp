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
#include <QTimer>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(memeWallpaper, "meme.wallpaper")

WallpaperPlayer::WallpaperPlayer(QObject *parent)
    : QObject(parent)
{
    m_window = new QWidget();
    m_window->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    m_window->setAttribute(Qt::WA_TranslucentBackground);
    m_window->setAttribute(Qt::WA_ShowWithoutActivating);

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

    QRect geo;
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) geo = screen->geometry();
    if (geo.isEmpty()) geo = QRect(0, 0, 1920, 1080);

    m_window->setGeometry(geo);
    m_videoWidget->setGeometry(0, 0, geo.width(), geo.height());
    qCInfo(memeWallpaper) << "Window geometry:" << geo;

    m_player->setSource(QUrl::fromLocalFile(path));
    m_window->show();
    m_player->play();

    // 延迟再设一次几何,以防 show 后窗口被 WM 改了大小
    QTimer::singleShot(500, this, [this, geo]() {
        QRect cur = m_window->geometry();
        if (cur.width() <= 1 || cur.height() <= 1) {
            m_window->setGeometry(geo);
            m_videoWidget->setGeometry(0, 0, geo.width(), geo.height());
            qCInfo(memeWallpaper) << "Re-applied geometry:" << geo;
        }
    });

    qCInfo(memeWallpaper) << "Playing video wallpaper:" << path;
}

void WallpaperPlayer::stop()
{
    m_player->stop();
    m_window->hide();
}
