// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "wallpaperplayer.h"
#include "memedconfig.h"

#include <QGuiApplication>
#include <QScreen>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QQuickView>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QFile>
#include <QTimer>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(memeWallpaper, "meme.wallpaper")

WallpaperPlayer::WallpaperPlayer(QObject *parent)
    : QObject(parent)
{
    m_player = new QMediaPlayer(this);
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
    delete m_view;
}

void WallpaperPlayer::ensureView()
{
    if (m_view) return;

    m_view = new QQuickView();
    m_view->setFlag(Qt::FramelessWindowHint);
    m_view->setFlag(Qt::WindowStaysOnBottomHint);
    m_view->setColor(Qt::black);

    // 用 QML 渲染视频帧
    m_view->setSource(QUrl(QStringLiteral("qrc:/meme/wallpaper.qml")));

    m_sink = new QVideoSink(this);
    m_player->setVideoSink(m_sink);

    // 把 VideoSink 的 videoFrame 传给 QML 的 Image
    auto *root = m_view->rootObject();
    if (root) {
        root->setProperty("videoSink", QVariant::fromValue(m_sink));
    }

    m_view->show();
    applyGeometry();

    // show 后立即再设一次大小,并持续监听 resize 事件
    QTimer::singleShot(100, this, [this]() { applyGeometry(); });
    QTimer::singleShot(500, this, [this]() { applyGeometry(); });

    connect(m_view, &QQuickView::widthChanged, this, [this](int w) {
        if (w <= 1) applyGeometry();
    });
    connect(m_view, &QQuickView::heightChanged, this, [this](int h) {
        if (h <= 1) applyGeometry();
    });
}

void WallpaperPlayer::applyGeometry()
{
    if (!m_view) return;

    QRect geo;
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) geo = screen->geometry();
    if (geo.isEmpty()) geo = QRect(0, 0, 1920, 1080);

    if (m_view->geometry() != geo) {
        m_view->setGeometry(geo);
        qCInfo(memeWallpaper) << "Applied geometry:" << geo;
    }
}

void WallpaperPlayer::setVideo(const QString &path)
{
    if (path.isEmpty() || !QFile::exists(path)) {
        qCWarning(memeWallpaper) << "Video not found:" << path;
        return;
    }

    ensureView();
    applyGeometry();

    m_player->setSource(QUrl::fromLocalFile(path));
    m_player->play();

    qCInfo(memeWallpaper) << "Playing video wallpaper:" << path;
}

void WallpaperPlayer::stop()
{
    m_player->stop();
    if (m_view) m_view->hide();
}
