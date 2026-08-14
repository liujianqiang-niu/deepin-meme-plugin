// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "wallpaperplayer.h"
#include "memedconfig.h"

#include <QApplication>
#include <QDBusConnection>
#include <QLoggingCategory>

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
    void SetWallpaper(const QString &path) { m_player->setVideo(path); }
    void Stop() { m_player->stop(); }

private:
    WallpaperPlayer *m_player;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("deepin-meme-daemon");

    qCInfo(memeDaemon) << "Starting...";

    WallpaperPlayer player;

    MemeDaemon daemon(&player);

    auto bus = QDBusConnection::sessionBus();
    bus.registerService(kDBusService);
    bus.registerObject(kDBusPath, &daemon, QDBusConnection::ExportAllSlots);

    qCInfo(memeDaemon) << "Started.";

    return app.exec();
}

#include "main.moc"
