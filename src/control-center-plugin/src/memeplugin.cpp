// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "memeplugin.h"
#include "memedconfig.h"

#include "dccfactory.h"

#include <QDir>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QStandardPaths>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(memePlugin, "meme.plugin")

static const char *kWallpaperDir = "/usr/share/deepin-meme-wallpapers";

MemePlugin::MemePlugin(QObject *parent)
    : QObject(parent)
{
    loadWallpaperList();

    meme::MemeDConfig cfg;
    if (cfg.isValid()) {
        m_enabled = cfg.enabled();
        m_currentVideo = cfg.currentVideo();
    }
}

MemePlugin::~MemePlugin() = default;

void MemePlugin::loadWallpaperList()
{
    m_wallpapers.clear();
    QDir dir(QString::fromUtf8(kWallpaperDir));
    if (!dir.exists()) return;

    const auto files = dir.entryList({"*.mp4"}, QDir::Files, QDir::Name);
    for (const QString &file : files) {
        WallpaperEntry entry;
        entry.name = file;
        entry.path = dir.absoluteFilePath(file);
        m_wallpapers.append(entry);
    }
}

bool MemePlugin::enabled() const { return m_enabled; }

void MemePlugin::setEnabled(bool e)
{
    if (m_enabled != e) {
        m_enabled = e;
        emit enabledChanged(e);

        meme::MemeDConfig cfg;
        cfg.setEnabled(e);

        if (!e) {
            QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(
                "org.deepin.meme.daemon", "/org/deepin/meme/daemon",
                "org.deepin.meme.daemon", "Stop"));
        } else if (!m_currentVideo.isEmpty()) {
            applyWallpaper(m_currentVideo);
        }
    }
}

QString MemePlugin::currentVideo() const { return m_currentVideo; }

void MemePlugin::setCurrentVideo(const QString &path)
{
    if (m_currentVideo != path) {
        m_currentVideo = path;
        emit currentVideoChanged(path);
        meme::MemeDConfig cfg;
        cfg.setCurrentVideo(path);
    }
}

QVariantList MemePlugin::wallpaperModel() const
{
    QVariantList model;
    for (const auto &w : m_wallpapers) {
        QVariantMap entry;
        entry[QStringLiteral("name")] = w.name;
        entry[QStringLiteral("path")] = w.path;
        model.append(entry);
    }
    return model;
}

void MemePlugin::applyWallpaper(const QString &path)
{
    setCurrentVideo(path);

    meme::MemeDConfig cfg;
    cfg.setEnabled(true);

    if (!m_enabled) {
        m_enabled = true;
        emit enabledChanged(true);
    }

    QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(
        "org.deepin.meme.daemon", "/org/deepin/meme/daemon",
        "org.deepin.meme.daemon", "SetWallpaper") << path);

    qCInfo(memePlugin) << "Applied wallpaper:" << path;
}

DCC_FACTORY_CLASS(MemePlugin)
#include "memeplugin.moc"
