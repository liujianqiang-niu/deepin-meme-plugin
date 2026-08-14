// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "wallpaperplayer.h"
#include "memedconfig.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QLoggingCategory>
#include <QProcessEnvironment>
#include <cstdlib>

Q_LOGGING_CATEGORY(memeDaemon, "meme.daemon")

static const char *kDBusService = "org.deepin.meme.daemon";
static const char *kDBusPath = "/org/deepin/meme/daemon";
static const char *kDBusInterface = "org.deepin.meme.daemon";

class MemeDaemon : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.meme.daemon")
public:
    explicit MemeDaemon(WallpaperPlayer *player, QObject *parent = nullptr)
        : QObject(parent), m_player(player)
    {
        meme::MemeDConfig cfg;
        if (cfg.isValid() && cfg.enabled()) {
            m_player->setVideo(cfg.currentVideo());
        }
    }

public Q_SLOTS:
    void SetWallpaper(const QString &path)
    {
        qCInfo(memeDaemon) << "SetWallpaper called:" << path;
        m_player->setVideo(path);
    }
    void Stop()
    {
        qCInfo(memeDaemon) << "Stop called";
        m_player->stop();
    }

private:
    WallpaperPlayer *m_player;
};

int main(int argc, char *argv[])
{
    // D-Bus 激活时可能没有 DISPLAY 环境变量,导致 QApplication 无法连接 X11。
    // 从 systemd 或 session 环境中继承。
    if (std::getenv("DISPLAY") == nullptr) {
        qputenv("DISPLAY", ":0");
    }

    QApplication app(argc, argv);
    app.setApplicationName("deepin-meme-daemon");

    qCInfo(memeDaemon) << "Starting...";

    WallpaperPlayer player;

    MemeDaemon daemon(&player);

    auto bus = QDBusConnection::sessionBus();
    if (!bus.registerService(kDBusService)) {
        qCWarning(memeDaemon) << "Failed to register D-Bus service:" << bus.lastError().message();
    }
    if (!bus.registerObject(kDBusPath, &daemon, QDBusConnection::ExportAllSlots)) {
        qCWarning(memeDaemon) << "Failed to register D-Bus object:" << bus.lastError().message();
    }

    qCInfo(memeDaemon) << "Started. D-Bus service:" << kDBusService;

    return app.exec();
}

#include "main.moc"
