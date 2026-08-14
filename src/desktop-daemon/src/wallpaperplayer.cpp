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
    qCInfo(memeWallpaper) << "WallpaperPlayer created";
}

WallpaperPlayer::~WallpaperPlayer()
{
    qCInfo(memeWallpaper) << "WallpaperPlayer destroyed";
    delete m_view;
}

void WallpaperPlayer::ensureView()
{
    if (m_view) return;

    qCInfo(memeWallpaper) << "Creating QQuickView...";

    m_view = new QQuickView();
    m_view->setFlag(Qt::FramelessWindowHint);
    m_view->setFlag(Qt::WindowStaysOnBottomHint);
    m_view->setColor(Qt::black);

    m_view->setSource(QUrl(QStringLiteral("qrc:/meme/wallpaper.qml")));

    if (m_view->status() != QQuickView::Ready) {
        qCWarning(memeWallpaper) << "QML load errors:" << m_view->errors();
    }

    auto *root = m_view->rootObject();
    if (!root) {
        qCWarning(memeWallpaper) << "QML root object is null!";
        return;
    }

    qCInfo(memeWallpaper) << "QML root object ready, showing window...";

    m_view->show();
    applyGeometry();

    QTimer::singleShot(100, this, [this]() {
        applyGeometry();
        m_view->raise();
        m_view->requestUpdate();
        qCInfo(memeWallpaper) << "Window state after 100ms:"
                              << "visible=" << m_view->isVisible()
                              << "geometry=" << m_view->geometry()
                              << "width=" << m_view->width()
                              << "height=" << m_view->height();
    });
    QTimer::singleShot(500, this, [this]() {
        applyGeometry();
        qCInfo(memeWallpaper) << "Window state after 500ms:"
                              << "visible=" << m_view->isVisible()
                              << "geometry=" << m_view->geometry();
    });

    connect(m_view, &QQuickView::widthChanged, this, [this](int w) {
        qCDebug(memeWallpaper) << "widthChanged:" << w;
        if (w <= 1) applyGeometry();
    });
    connect(m_view, &QQuickView::heightChanged, this, [this](int h) {
        qCDebug(memeWallpaper) << "heightChanged:" << h;
        if (h <= 1) applyGeometry();
    });
    connect(m_view, &QQuickView::visibilityChanged, this, [this](QWindow::Visibility v) {
        qCInfo(memeWallpaper) << "visibilityChanged:" << v;
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
    qCInfo(memeWallpaper) << "setVideo called:" << path;

    if (path.isEmpty()) {
        qCWarning(memeWallpaper) << "Path is empty";
        return;
    }
    if (!QFile::exists(path)) {
        qCWarning(memeWallpaper) << "File not found:" << path;
        return;
    }

    ensureView();
    applyGeometry();

    auto *root = m_view ? m_view->rootObject() : nullptr;
    if (!root) {
        qCWarning(memeWallpaper) << "QML root object null, cannot set video source";
        return;
    }

    root->setProperty("videoSource", path);
    qCInfo(memeWallpaper) << "Video source set, window visible:" << m_view->isVisible()
                          << "geometry:" << m_view->geometry();
}

void WallpaperPlayer::stop()
{
    qCInfo(memeWallpaper) << "stop called";
    if (m_view) {
        auto *root = m_view->rootObject();
        if (root) root->setProperty("videoSource", "");
        m_view->hide();
        qCInfo(memeWallpaper) << "Window hidden";
    }
}
