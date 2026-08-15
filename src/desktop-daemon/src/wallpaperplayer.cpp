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
#include <QDBusInterface>
#include <QDBusMessage>
#include <QLoggingCategory>

#ifndef QT_NO_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

Q_LOGGING_CATEGORY(memeWallpaper, "meme.wallpaper")

static const char *kTransparentWallpaper = "/usr/share/deepin-meme-wallpapers/transparent.png";

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
    restoreStaticWallpaper();
    delete m_player;
    delete m_widget;
}

void WallpaperPlayer::ensureWidget()
{
    if (m_widget) return;

    qCInfo(memeWallpaper) << "Creating video display widget...";

    m_widget = new VideoDisplayWidget();
    m_widget->setWindowFlags(Qt::FramelessWindowHint);

    m_player = new QMediaPlayer(this);
    m_sink = new QVideoSink(this);
    m_player->setVideoSink(m_sink);

    connect(m_sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame) {
        m_widget->presentFrame(frame);
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this, [](QMediaPlayer::Error error, const QString &errorString) {
        qCWarning(memeWallpaper) << "Media error:" << error << errorString;
    });

    // Set _NET_WM_WINDOW_TYPE_DESKTOP so KWin places the window in the desktop
    // layer (same as dde-desktop), making it behave as wallpaper background.
    // Also add _NET_WM_STATE_SKIP_TASKBAR and _NET_WM_STATE_SKIP_PAGER to hide
    // it from taskbar and pager. Must be set after native window creation
    // (winId()) but before show() — KWin reads these at map time.
    (void)m_widget->winId();
#ifndef QT_NO_X11
    {
        Display *dpy = XOpenDisplay(nullptr);
        if (dpy) {
            Window winId = static_cast<Window>(m_widget->winId());

            Atom netWmWindowType = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
            Atom netWmWindowTypeDesktop = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
            XChangeProperty(dpy, winId, netWmWindowType, XA_ATOM, 32, PropModeReplace,
                            reinterpret_cast<unsigned char *>(&netWmWindowTypeDesktop), 1);

            Atom netWmState = XInternAtom(dpy, "_NET_WM_STATE", False);
            Atom skipTaskbar = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False);
            Atom skipPager = XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False);
            Atom states[] = { skipTaskbar, skipPager };
            XChangeProperty(dpy, winId, netWmState, XA_ATOM, 32, PropModeReplace,
                            reinterpret_cast<unsigned char *>(states), 2);

            XFlush(dpy);
            XCloseDisplay(dpy);
            qCInfo(memeWallpaper) << "Set DESKTOP window type + SKIP_TASKBAR/PAGER on" << Qt::hex << winId;
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

void WallpaperPlayer::setStaticWallpaperTransparent()
{
    if (m_wallpaperReplaced) return;

    auto msg = QDBusMessage::createMethodCall(
        "org.deepin.dde.Appearance1", "/org/deepin/dde/Appearance1",
        "org.deepin.dde.Appearance1", "GetCurrentWorkspaceBackground");
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        QString current = reply.arguments().first().toString();
        if (current.contains("transparent.png")) {
            qCInfo(memeWallpaper) << "Current wallpaper is already transparent, not saving";
        } else {
            m_savedWallpaper = current;
            qCInfo(memeWallpaper) << "Saved current wallpaper:" << m_savedWallpaper;
        }
    }

    msg = QDBusMessage::createMethodCall(
        "org.deepin.dde.Appearance1", "/org/deepin/dde/Appearance1",
        "org.deepin.dde.Appearance1", "SetCurrentWorkspaceBackground");
    msg << QString("file://%1").arg(kTransparentWallpaper);
    QDBusConnection::sessionBus().call(msg);
    m_wallpaperReplaced = true;
    qCInfo(memeWallpaper) << "Set static wallpaper to transparent PNG";
}

void WallpaperPlayer::restoreStaticWallpaper()
{
    if (!m_wallpaperReplaced) return;

    auto msg = QDBusMessage::createMethodCall(
        "org.deepin.dde.Appearance1", "/org/deepin/dde/Appearance1",
        "org.deepin.dde.Appearance1", "SetCurrentWorkspaceBackground");
    if (m_savedWallpaper.isEmpty()) {
        msg << "file:///usr/share/backgrounds/default_background.jpg";
    } else {
        msg << m_savedWallpaper;
    }
    QDBusConnection::sessionBus().call(msg);
    m_wallpaperReplaced = false;
    qCInfo(memeWallpaper) << "Restored static wallpaper:" << m_savedWallpaper;
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
    setStaticWallpaperTransparent();

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
    restoreStaticWallpaper();
}

#include "wallpaperplayer.moc"
