// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
// 桌面特效守护进程入口

#include "effectoverlay.h"
#include "fileoperationmonitor.h"
#include "themeresolver.h"
#include "effectplayer.h"
#include "wallpapermanager.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(memeDaemon, "meme.daemon")

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("deepin-meme-daemon");
    app.setOrganizationName("deepin");
    app.setApplicationDisplayName("Deepin Meme Daemon");

    qCInfo(memeDaemon) << "Deepin Meme Daemon starting...";

    // 1. 主题解析器
    meme::ThemeResolver themeResolver;
    themeResolver.scanThemes();
    if (themeResolver.availableThemes().isEmpty()) {
        qCWarning(memeDaemon) << "No meme themes found, daemon will run idle.";
    }

    // 2. 特效播放器
    EffectPlayer effectPlayer(&themeResolver);

    // 2.5 动态壁纸管理器
    WallpaperManager wallpaperManager(&themeResolver);

    // 3. 特效叠加层
    EffectOverlay overlay(&effectPlayer, &wallpaperManager);

    // 4. 文件操作监听器
    FileOperationMonitor monitor;
    QObject::connect(&monitor, &FileOperationMonitor::fileOperationDetected,
        &overlay, [&overlay](const QString &operationType, const QString &filePath) {
            overlay.triggerEffect(operationType, filePath);
        });

    monitor.start();

    // 5. 注册 D-Bus 服务
    auto bus = QDBusConnection::sessionBus();
    if (!bus.registerService("org.deepin.meme.daemon")) {
        qCWarning(memeDaemon) << "Failed to register D-Bus service:" << bus.lastError().message();
    }
    if (!bus.registerObject("/org/deepin/meme/daemon", &overlay,
            QDBusConnection::ExportAllSlots)) {
        qCWarning(memeDaemon) << "Failed to register D-Bus object:" << bus.lastError().message();
    }

    qCInfo(memeDaemon) << "Deepin Meme Daemon started.";

    return app.exec();
}
