// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine_p.h"
#include "util/event_helper.h"
#include "util/menu_eventinterface_helper.h"
#include "menu.h"

#include <dfm-base/dfm_desktop_defines.h>

#include <QDir>
#include <QStandardPaths>
#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QFileInfo>
#include <QGuiApplication>
#include <QScreen>
#include <QTimer>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QWindow>
#include <atomic>
#include <memory>

using namespace ddplugin_meme;
DFMBASE_USE_NAMESPACE

#define CanvasCoreSubscribe(topic, func) \
    dpfSignalDispatcher->subscribe("ddplugin_core", QT_STRINGIFY2(topic), this, func)

#define CanvasCoreUnsubscribe(topic, func) \
    dpfSignalDispatcher->unsubscribe("ddplugin_core", QT_STRINGIFY2(topic), this, func)

static QString getScreenName(QWidget *win)
{
    return win->property(DesktopFrameProperty::kPropScreenName).toString();
}

static QMap<QString, QWidget *> rootMap()
{
    QList<QWidget *> root = ddplugin_meme_util::desktopFrameRootWindows();
    QMap<QString, QWidget *> ret;
    for (QWidget *win : root) {
        QString name = getScreenName(win);
        if (name.isEmpty())
            continue;
        ret.insert(name, win);
    }
    return ret;
}

static QUrl firstVideoInDir(const QDir &dir)
{
    static const QStringList filters {
        QStringLiteral("*.mp4"), QStringLiteral("*.mkv"), QStringLiteral("*.webm"),
        QStringLiteral("*.avi"), QStringLiteral("*.mov"), QStringLiteral("*.m4v")
    };
    for (const QFileInfo &file : dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name)) {
        if (file.fileName().compare(QStringLiteral("current.mp4"), Qt::CaseInsensitive) == 0)
            continue;
        const QString key = file.canonicalFilePath();
        if (!key.isEmpty())
            return QUrl::fromLocalFile(key);
    }
    return {};
}

WallpaperEnginePrivate::WallpaperEnginePrivate(WallpaperEngine *qq)
    : q(qq)
{
}

bool WallpaperEnginePrivate::isScreenActive(const QString &screenName) const
{
    Q_UNUSED(screenName)
    // DConfig 架构下无 per-screen 开关；全局 enabled 即所有屏激活
    return cfg && cfg->enabled();
}

QUrl WallpaperEnginePrivate::videoForScreen(const QString &screenName) const
{
    Q_UNUSED(screenName)
    // DConfig 架构：单一 currentVideo 应用于所有屏
    const QString chosen = cfg ? cfg->currentVideo().trimmed() : QString();
    if (!chosen.isEmpty()) {
        QFileInfo fi(chosen);
        if (fi.exists() && fi.isFile())
            return QUrl::fromLocalFile(fi.canonicalFilePath());
    }

    // 回退：扫描预置目录
    const QDir root(sourcePath());
    return firstVideoInDir(root);
}

int WallpaperEnginePrivate::maxScreenWidth() const
{
    // 物理像素：逻辑宽 × DPR（125% 时 2048→2560）
    int maxW = 0;
    for (QScreen *s : QGuiApplication::screens()) {
        if (!s)
            continue;
        const qreal dpr = s->devicePixelRatio();
        maxW = qMax(maxW, qRound(s->size().width() * dpr));
    }
    return maxW > 0 ? maxW : 1920;
}

int WallpaperEnginePrivate::maxWidthForScreens(const QList<QString> &screens) const
{
    int maxW = 0;
    // 优先用 QScreen 名匹配（比 widget 尚未 layout 时的 width 准）
    for (QScreen *s : QGuiApplication::screens()) {
        if (!s)
            continue;
        const QString n = s->name();
        if (!screens.contains(n))
            continue;
        maxW = qMax(maxW, qMax(1, qRound(s->size().width() * s->devicePixelRatio())));
    }
    // 回退：控件几何
    if (maxW <= 0) {
        for (const QString &name : screens) {
            VideoProxyPointer proxy = widgets.value(name);
            if (proxy.isNull())
                continue;
            qreal dpr = QGuiApplication::primaryScreen()
                    ? QGuiApplication::primaryScreen()->devicePixelRatio() : 1.0;
            maxW = qMax(maxW, qMax(1, qRound(proxy->width() * dpr)));
        }
    }
    if (maxW <= 0)
        maxW = maxScreenWidth();
    maxW &= ~1;
    return maxW;
}

void WallpaperEnginePrivate::setPlaybackSuspended(bool suspended, const char *reason)
{
    if (playbackSuspended == suspended)
        return;
    playbackSuspended = suspended;
    qInfo() << "[meme-wallpaper] playback" << (suspended ? "SUSPEND" : "RESUME")
            << "reason=" << reason;
    if (!cfg || !cfg->enabled())
        return;
    if (suspended) {
        stopSharedDecoders();
        for (const VideoProxyPointer &w : widgets) {
            if (!w.isNull())
                w->stop();
        }
    } else {
        startDebounce->start();
    }
}

void WallpaperEnginePrivate::setupPowerHooks()
{
    // 锁屏：Deepin LockFront（信号名因版本可能不同，多路尝试）
    QDBusConnection session = QDBusConnection::sessionBus();
    const char *lockSvc = "org.deepin.dde.LockFront1";
    const char *lockPath = "/org/deepin/dde/LockFront1";
    for (const char *sig : { "Locked", "Unlocked", "Visible" }) {
        session.connect(QString::fromLatin1(lockSvc), QString::fromLatin1(lockPath),
                        QString::fromLatin1(lockSvc), QString::fromLatin1(sig),
                        q, SLOT(onSessionLockSignal()));
    }
    // 通用 ScreenSaver ActiveChanged(bool)
    session.connect(QStringLiteral("org.freedesktop.ScreenSaver"),
                    QStringLiteral("/ScreenSaver"),
                    QStringLiteral("org.freedesktop.ScreenSaver"),
                    QStringLiteral("ActiveChanged"),
                    q, SLOT(onScreenSaverActiveChanged(bool)));
    session.connect(QStringLiteral("org.deepin.dde.ScreenSaver1"),
                    QStringLiteral("/org/deepin/dde/ScreenSaver1"),
                    QStringLiteral("org.deepin.dde.ScreenSaver1"),
                    QStringLiteral("ActiveChanged"),
                    q, SLOT(onScreenSaverActiveChanged(bool)));

    // 周期性：控件全不可见则停（被其它全屏盖住/构建间隙）
    visibilityTimer = new QTimer(q);
    visibilityTimer->setInterval(2000);
    QObject::connect(visibilityTimer, &QTimer::timeout, q, [this]() {
        if (!cfg || !cfg->enabled() || sessionLocked || screenSaverActive)
            return;
        bool anyVisible = false;
        for (auto it = widgets.constBegin(); it != widgets.constEnd(); ++it) {
            if (!it.value().isNull() && it.value()->isVisible()
                && isScreenActive(it.key())) {
                anyVisible = true;
                break;
            }
        }
        if (!anyVisible && !decoders.isEmpty())
            setPlaybackSuspended(true, "all-widgets-hidden");
        else if (anyVisible && playbackSuspended && !sessionLocked && !screenSaverActive)
            setPlaybackSuspended(false, "widget-visible");
    });
    visibilityTimer->start();

    QObject::connect(qApp, &QGuiApplication::applicationStateChanged, q,
                     [this](Qt::ApplicationState st) {
        // 会话被挂起/完全隐藏时停解码；Inactive 不停（切窗口仍要壁纸）
        if (st == Qt::ApplicationSuspended) {
            setPlaybackSuspended(true, "app-suspended");
        } else if (st == Qt::ApplicationActive
                   && playbackSuspended
                   && !sessionLocked
                   && !screenSaverActive) {
            bool anyVisible = false;
            for (auto it = widgets.constBegin(); it != widgets.constEnd(); ++it) {
                if (!it.value().isNull() && it.value()->isVisible() && isScreenActive(it.key())) {
                    anyVisible = true;
                    break;
                }
            }
            if (anyVisible)
                setPlaybackSuspended(false, "app-active");
        }
    });
}

PlayOptions WallpaperEnginePrivate::playOptions() const
{
    PlayOptions opt;
    opt.mode = cfg ? cfg->decodeMode() : DecodeMode::Software;
    opt.smooth = SmoothLevel::High;       // DConfig 未暴露，默认高平滑
    opt.fill = cfg ? cfg->fillMode() : FillMode::Fill;
    opt.speed = 1.0;                      // DConfig 未暴露，默认原速
    opt.fps = 0.0;                        // 0 = 跟随源帧率
    opt.maxWidth = maxScreenWidth();      // 按最大屏物理宽出图
    return opt;
}

VideoProxyPointer WallpaperEnginePrivate::createWidget(QWidget *root)
{
    // 构造即 parent：嵌入桌面，禁止独立顶层窗
    VideoProxyPointer bwp(new VideoProxy(root));
    bwp->setGeometry(relativeGeometry(root->geometry()));
    const QString name = getScreenName(root);
    bwp->setProperty(DesktopFrameProperty::kPropScreenName, name);
    bwp->setProperty(DesktopFrameProperty::kPropWidgetName, "meme-wallpaper");
    bwp->setProperty(DesktopFrameProperty::kPropWidgetLevel, 5.1);
    // 从 DConfig 下发填充模式
    if (cfg)
        bwp->setFillMode(cfg->fillMode());
    return bwp;
}

void WallpaperEnginePrivate::setBackgroundVisible(bool v)
{
    for (QWidget *root : ddplugin_meme_util::desktopFrameRootWindows()) {
        const QString name = getScreenName(root);
        if (v || isScreenActive(name))
            setBackgroundVisibleFor(name, v);
    }
}

void WallpaperEnginePrivate::setBackgroundVisibleFor(const QString &screenName, bool v)
{
    auto map = rootMap();
    QWidget *root = map.value(screenName);
    if (!root)
        return;
    for (QObject *obj : root->children()) {
        if (auto *wid = qobject_cast<QWidget *>(obj)) {
            if (wid->property(DesktopFrameProperty::kPropWidgetName).toString() == QLatin1String("background"))
                wid->setVisible(v);
        }
    }
}

QString WallpaperEnginePrivate::sourcePath() const
{
    // 预置视频目录（安装时由 CMake install 部署）
    return QStringLiteral("/usr/share/deepin-meme-wallpapers");
}

void WallpaperEnginePrivate::stopSharedDecoders()
{
    for (VideoDecoder *dec : decoders) {
        if (!dec)
            continue;
        dec->requestStop();
        dec->wait(4000);
        dec->deleteLater();
    }
    decoders.clear();
}

void WallpaperEnginePrivate::startSharedDecoders()
{
    stopSharedDecoders();
    PlayOptions popt = playOptions();
    DecodeOptions opt;
    opt.mode = popt.mode;
    opt.smooth = popt.smooth;
    opt.speed = popt.speed;
    opt.fps = popt.fps;
    opt.maxWidth = popt.maxWidth;
    opt.preferNv12 = false; // 嵌入 QWidget：解码侧 RGB

    QHash<QUrl, QList<QString>> urlScreens;
    for (auto it = screenVideo.constBegin(); it != screenVideo.constEnd(); ++it) {
        if (!isScreenActive(it.key()) || it.value().isEmpty())
            continue;
        urlScreens[it.value()].append(it.key());
    }

    for (auto it = urlScreens.begin(); it != urlScreens.end(); ++it) {
        auto *decoder = new VideoDecoder(q);
        DecodeOptions decOpt = opt;
        const QList<QString> screens = it.value();
        // 按实际挂接屏控件宽度出图（双屏 1920+2560 时 1920 屏不再被迫 2560 宽）
        const int geoW = maxWidthForScreens(screens);
        if (geoW > 0 && (decOpt.maxWidth <= 0 || geoW < decOpt.maxWidth))
            decOpt.maxWidth = geoW;
        decoder->setOptions(decOpt);
        decoder->setPlaylist({ it.key() });
        // 主线程只 fromImage 一次；处理完 releaseFrameSlot，解码才推下一帧
        QObject::connect(decoder, &VideoDecoder::frameReady, q,
                         [this, screens, decoder](const VideoFrame &frame) {
            if (frame.isNull()) {
                decoder->releaseFrameSlot();
                return;
            }
            QList<VideoProxyPointer> targets;
            targets.reserve(screens.size());
            for (const QString &name : screens) {
                VideoProxyPointer proxy = widgets.value(name);
                if (!proxy.isNull() && proxy->isVisible())
                    targets.append(proxy);
            }
            if (targets.isEmpty()) {
                decoder->releaseFrameSlot();
                return;
            }
            // 架构：解码线程已是 RGB；主线程只 fromImage 一次，双屏共享 QPixmap + drawPixmap
            if (frame.format != VideoFrame::Format::Rgb32 || frame.rgb.isNull()) {
                decoder->releaseFrameSlot();
                return;
            }
            const QPixmap pm = QPixmap::fromImage(frame.rgb);
            const int w = frame.width > 0 ? frame.width : pm.width();
            const int h = frame.height > 0 ? frame.height : pm.height();
            for (const VideoProxyPointer &proxy : targets)
                proxy->presentPixmap(pm, w, h, {});
            decoder->releaseFrameSlot();
        }, Qt::QueuedConnection);
        decoder->start();
        decoders.insert(it.key(), decoder);
        qInfo() << "[meme-wallpaper] shared decoder" << it.key() << "screens" << screens
                << "maxW" << decOpt.maxWidth << "fps" << decOpt.fps;
    }
}

void WallpaperEnginePrivate::stopPlayers()
{
    if (startDebounce)
        startDebounce->stop();
    for (const VideoProxyPointer &w : widgets)
        if (!w.isNull())
            w->stop();
    stopSharedDecoders();
}

void WallpaperEnginePrivate::startPlayers()
{
    if (playbackSuspended || sessionLocked || screenSaverActive) {
        qInfo() << "[meme-wallpaper] startPlayers skipped (suspended)";
        return;
    }
    stopSharedDecoders();
    for (const VideoProxyPointer &w : widgets)
        if (!w.isNull())
            w->show();
    startSharedDecoders();
    for (auto it = screenVideo.begin(); it != screenVideo.end(); ++it) {
        if (isScreenActive(it.key()))
            setBackgroundVisibleFor(it.key(), false);
    }
}

WallpaperEngine::WallpaperEngine(QObject *parent)
    : QObject(parent)
    , d(new WallpaperEnginePrivate(this))
{
    d->cfg = new MemeConfig(this);
    d->startDebounce = new QTimer(this);
    d->startDebounce->setSingleShot(true);
    d->startDebounce->setInterval(300);
    connect(d->startDebounce, &QTimer::timeout, this, [this]() {
        if (d->cfg && d->cfg->enabled())
            d->startPlayers();
    });
    d->setupPowerHooks();
}

void WallpaperEngine::onSessionLockSignal()
{
    // Visible/Locked/Unlocked 无参：轮询 Active 属性更稳
    QDBusInterface lock(QStringLiteral("org.deepin.dde.LockFront1"),
                        QStringLiteral("/org/deepin/dde/LockFront1"),
                        QStringLiteral("org.deepin.dde.LockFront1"),
                        QDBusConnection::sessionBus());
    bool locked = false;
    if (lock.isValid()) {
        const QVariant v = lock.property("Visible");
        if (v.isValid())
            locked = v.toBool();
    }
    d->sessionLocked = locked;
    d->setPlaybackSuspended(locked || d->screenSaverActive,
                            locked ? "session-lock" : "session-unlock");
}

void WallpaperEngine::onScreenSaverActiveChanged(bool active)
{
    d->screenSaverActive = active;
    d->setPlaybackSuspended(d->sessionLocked || active,
                            active ? "screensaver-on" : "screensaver-off");
}

WallpaperEngine::~WallpaperEngine()
{
    turnOff();
    delete d;
    d = nullptr;
}

bool WallpaperEngine::init()
{
    try {
        if (!d->cfg || !d->cfg->isValid()) {
            qWarning() << "[meme-wallpaper] DConfig not valid, engine disabled";
            return false;
        }

        if (!registerMenu())
            dpfSignalDispatcher->subscribe("dfmplugin_menu", "signal_MenuScene_SceneAdded",
                                           this, &WallpaperEngine::registerMenu);

        connect(d->cfg, &MemeConfig::configChanged, this, &WallpaperEngine::onOptionsChanged);

        if (d->cfg->enabled())
            turnOn(true);
    } catch (const std::exception &ex) {
        qWarning() << "[meme-wallpaper] init exception:" << ex.what();
        return false;
    } catch (...) {
        qWarning() << "[meme-wallpaper] init unknown exception";
        return false;
    }
    return true;
}

void WallpaperEngine::turnOn(bool b)
{
    if (d->watcher)
        return;

    CanvasCoreSubscribe(signal_DesktopFrame_WindowShowed, &WallpaperEngine::play);
    CanvasCoreSubscribe(signal_DesktopFrame_WindowBuilded, &WallpaperEngine::build);
    CanvasCoreSubscribe(signal_DesktopFrame_GeometryChanged, &WallpaperEngine::geometryChanged);
    CanvasCoreSubscribe(signal_DesktopFrame_WindowAboutToBeBuilded, &WallpaperEngine::onDetachWindows);

    // 监视预置视频目录变化
    d->watcher = new QFileSystemWatcher(this);
    const QString src = d->sourcePath();
    d->watcher->addPath(src);
    connect(d->watcher, &QFileSystemWatcher::directoryChanged, this, &WallpaperEngine::refreshSource);

    refreshSource();
    if (b) {
        build();
        show();
    }
}

void WallpaperEngine::turnOff()
{
    CanvasCoreUnsubscribe(signal_DesktopFrame_WindowShowed, &WallpaperEngine::play);
    CanvasCoreUnsubscribe(signal_DesktopFrame_WindowBuilded, &WallpaperEngine::build);
    CanvasCoreUnsubscribe(signal_DesktopFrame_WindowAboutToBeBuilded, &WallpaperEngine::onDetachWindows);
    CanvasCoreUnsubscribe(signal_DesktopFrame_GeometryChanged, &WallpaperEngine::geometryChanged);

    delete d->watcher;
    d->watcher = nullptr;

    d->stopPlayers();
    d->widgets.clear();
    d->screenVideo.clear();
    d->setBackgroundVisible(true);
}

void WallpaperEngine::refreshSource()
{
    d->screenVideo.clear();
    for (QWidget *win : ddplugin_meme_util::desktopFrameRootWindows()) {
        const QString name = getScreenName(win);
        if (name.isEmpty() || !d->isScreenActive(name))
            continue;
        const QUrl url = d->videoForScreen(name);
        if (!url.isEmpty())
            d->screenVideo.insert(name, url);
    }

    qWarning() << "[meme-wallpaper] screenVideo:" << d->screenVideo;
    if (d->cfg && d->cfg->enabled())
        d->startDebounce->start();
}

void WallpaperEngine::build()
{
    QList<QWidget *> root = ddplugin_meme_util::desktopFrameRootWindows();
    QMap<QString, QWidget *> alive;

    for (QWidget *win : root) {
        const QString screenName = getScreenName(win);
        if (screenName.isEmpty())
            continue;
        alive.insert(screenName, win);

        if (!d->isScreenActive(screenName)) {
            if (auto old = d->widgets.take(screenName))
                old->stop();
            d->screenVideo.remove(screenName);
            d->setBackgroundVisibleFor(screenName, true);
            continue;
        }

        VideoProxyPointer bwp = d->widgets.value(screenName);
        if (!bwp.isNull()) {
            bwp->setParent(win);
            bwp->setGeometry(d->relativeGeometry(win->geometry()));
        } else {
            bwp = d->createWidget(win);
            d->widgets.insert(screenName, bwp);
        }

        const QUrl url = d->videoForScreen(screenName);
        if (!url.isEmpty())
            d->screenVideo.insert(screenName, url);
    }

    for (const QString &sp : d->widgets.keys()) {
        if (!alive.contains(sp) || !d->isScreenActive(sp)) {
            if (auto old = d->widgets.take(sp))
                old->stop();
        }
    }

    if (d->cfg && d->cfg->enabled())
        d->startDebounce->start();
}

void WallpaperEngine::onDetachWindows()
{
    for (const VideoProxyPointer &bwp : d->widgets.values()) {
        if (!bwp.isNull()) {
            bwp->stop();
            bwp->setParent(nullptr);
        }
    }
}

void WallpaperEngine::geometryChanged()
{
    build();
    auto winMap = rootMap();
    for (auto it = d->widgets.begin(); it != d->widgets.end(); ++it) {
        auto *win = winMap.value(it.key());
        if (!win || it.value().isNull())
            continue;
        it.value()->setGeometry(d->relativeGeometry(win->geometry()));
    }
    play();
}

void WallpaperEngine::play()
{
    if (!d->cfg || !d->cfg->enabled())
        return;

    show();

    for (QWidget *root : ddplugin_meme_util::desktopFrameRootWindows()) {
        const QString name = getScreenName(root);
        const bool active = d->isScreenActive(name) && d->widgets.contains(name);
        if (!active) {
            d->setBackgroundVisibleFor(name, true);
            continue;
        }
        d->setBackgroundVisibleFor(name, false);
    }

    d->startDebounce->start();
}

void WallpaperEngine::show()
{
    dpfSlotChannel->push("ddplugin_core", "slot_DesktopFrame_LayoutWidget");
    for (const VideoProxyPointer &bwp : d->widgets.values()) {
        if (bwp.isNull())
            continue;
        bwp->show();
    }
}

bool WallpaperEngine::registerMenu()
{
    if (!dfmplugin_menu_util::menuSceneContains("CanvasMenu"))
        return false;

    dfmplugin_menu_util::menuSceneRegisterScene(MemeWallpaperMenuCreator::name(),
                                                 new MemeWallpaperMenuCreator());
    dfmplugin_menu_util::menuSceneBind(MemeWallpaperMenuCreator::name(), "CanvasMenu");
    dpfSignalDispatcher->unsubscribe("dfmplugin_menu", "signal_MenuScene_SceneAdded",
                                     this, &WallpaperEngine::registerMenu);
    return true;
}

void WallpaperEngine::checkResouce()
{
    if (!d->screenVideo.isEmpty())
        return;

    const QString text = tr("Please add the video file to %0").arg(d->sourcePath());
    QDBusInterface notify("org.freedesktop.Notifications",
                          "/org/freedesktop/Notifications",
                          "org.freedesktop.Notifications");
    notify.setTimeout(1000);
    notify.asyncCall(QString("Notify"),
                     QString("Meme Wallpaper"),
                     static_cast<uint>(0),
                     QString("deepin-toggle-desktop"),
                     text,
                     QString(), QStringList(), QVariantMap(), 5000);
}

void WallpaperEngine::catchImage(const QImage &img)
{
    Q_UNUSED(img)
}

void WallpaperEngine::onOptionsChanged()
{
    if (!d->cfg)
        return;

    if (!d->cfg->enabled()) {
        turnOff();
        return;
    }

    const FillMode fill = d->cfg->fillMode();
    for (const VideoProxyPointer &w : d->widgets) {
        if (!w.isNull())
            w->setFillMode(fill);
    }

    build();
    play();
}
