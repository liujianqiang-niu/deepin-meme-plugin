// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "wallpaperplayer.h"

#include <QGuiApplication>
#include <QScreen>
#include <QWidget>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <QPainter>
#include <QPaintEvent>
#include <QFile>
#include <QTimer>
#include <QLoggingCategory>

#ifndef QT_NO_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

Q_LOGGING_CATEGORY(memeWallpaper, "meme.wallpaper")

class VideoDisplayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoDisplayWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setAttribute(Qt::WA_OpaquePaintEvent);
        setAttribute(Qt::WA_TranslucentBackground, false);
        setAutoFillBackground(false);
    }

    void presentFrame(const QVideoFrame &frame)
    {
        m_frame = frame;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        QImage image = m_frame.toImage();
        if (!image.isNull()) {
            painter.drawImage(rect(), image.convertToFormat(QImage::Format_RGB888));
        } else {
            painter.fillRect(rect(), Qt::black);
        }
    }

private:
    QVideoFrame m_frame;
};

WallpaperPlayer::WallpaperPlayer(QObject *parent)
    : QObject(parent)
{
    qCInfo(memeWallpaper) << "WallpaperPlayer created";
}

WallpaperPlayer::~WallpaperPlayer()
{
    qCInfo(memeWallpaper) << "WallpaperPlayer destroyed";
    delete m_player;
    delete m_widget;
}

void WallpaperPlayer::ensureWidget()
{
    if (m_widget) return;

    qCInfo(memeWallpaper) << "Creating video display widget...";

    m_widget = new VideoDisplayWidget();
    m_widget->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);

    m_player = new QMediaPlayer(this);
    m_sink = new QVideoSink(this);
    m_player->setVideoSink(m_sink);

    connect(m_sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame) {
        m_widget->presentFrame(frame);
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this, [](QMediaPlayer::Error error, const QString &errorString) {
        qCWarning(memeWallpaper) << "Media error:" << error << errorString;
    });

    // Qt::FramelessWindowHint makes KWin add _KDE_NET_WM_WINDOW_TYPE_OVERRIDE,
    // which causes the compositor to skip the window entirely. Force NORMAL type
    // via X11 so the compositor composites the painted video frames. Must be
    // done after native window creation (winId()) but before show().
    (void)m_widget->winId();
#ifndef QT_NO_X11
    {
        Display *dpy = XOpenDisplay(nullptr);
        if (dpy) {
            Window winId = static_cast<Window>(m_widget->winId());
            Atom netWmWindowType = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
            Atom netWmWindowTypeNormal = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_NORMAL", False);
            XChangeProperty(dpy, winId, netWmWindowType, XA_ATOM, 32, PropModeReplace,
                            reinterpret_cast<unsigned char *>(&netWmWindowTypeNormal), 1);
            XFlush(dpy);
            XCloseDisplay(dpy);
            qCInfo(memeWallpaper) << "Set NORMAL window type on" << Qt::hex << winId;
        }
    }
#endif

    m_widget->show();
    applyGeometry();

    QTimer::singleShot(100, this, [this]() {
        applyGeometry();
        m_widget->lower();
        m_widget->repaint();
    });
    QTimer::singleShot(500, this, [this]() {
        applyGeometry();
    });
}

void WallpaperPlayer::applyGeometry()
{
    if (!m_widget) return;

    QRect geo;
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) geo = screen->geometry();
    if (geo.isEmpty()) geo = QRect(0, 0, 1920, 1080);

    if (m_widget->geometry() != geo) {
        m_widget->setGeometry(geo);
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

    ensureWidget();
    applyGeometry();

    m_player->setSource(QUrl::fromLocalFile(path));
    m_player->play();

    qCInfo(memeWallpaper) << "Playing:" << path;
}

void WallpaperPlayer::stop()
{
    qCInfo(memeWallpaper) << "stop called";
    if (m_player) {
        m_player->stop();
    }
    if (m_widget) {
        m_widget->hide();
        qCInfo(memeWallpaper) << "Window hidden";
    }
}

#include "wallpaperplayer.moc"
