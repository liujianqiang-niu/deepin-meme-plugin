// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "wallpaperplayer.h"
#include "memedconfig.h"

#include <QGuiApplication>
#include <QScreen>
#include <QQuickView>
#include <QQuickItem>
#include <QFile>
#include <QTimer>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(memeWallpaper, "meme.wallpaper")

WallpaperPlayer::WallpaperPlayer(QObject *parent)
    : QObject(parent)
{
}

WallpaperPlayer::~WallpaperPlayer()
{
    delete m_view;
}

void WallpaperPlayer::ensureView()
{
    if (m_view) return;

    m_view = new QQuickView();
    m_view->setFlag(Qt::FramelessWindowHint);
    m_view->setFlag(Qt::WindowStaysOnBottomHint);
    m_view->setColor(Qt::black);
    m_view->setSource(QUrl(QStringLiteral("qrc:/meme/wallpaper.qml")));
    m_view->show();
    applyGeometry();

    QTimer::singleShot(100, this, [this]() {
        applyGeometry();
        m_view->raise();
        m_view->requestUpdate();
    });
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

    auto *root = m_view->rootObject();
    if (root) {
        root->setProperty("videoSource", path);
    }

    qCInfo(memeWallpaper) << "Playing video wallpaper:" << path;
}

void WallpaperPlayer::stop()
{
    if (m_view) {
        auto *root = m_view->rootObject();
        if (root) root->setProperty("videoSource", "");
        m_view->hide();
    }
}
