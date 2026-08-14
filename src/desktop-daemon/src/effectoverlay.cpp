// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "effectoverlay.h"
#include "effectplayer.h"
#include "wallpapermanager.h"
#include "themeresolver.h"
#include "memedconfig.h"

#include <QGuiApplication>
#include <QScreen>
#include <QDBusInterface>
#include <QDBusReply>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(memeOverlay, "meme.overlay")

EffectOverlay::EffectOverlay(EffectPlayer *player, WallpaperManager *wallpaperMgr, QObject *parent)
    : QObject(parent), m_player(player), m_wallpaperManager(wallpaperMgr)
{
    meme::MemeDConfig dconfig;
    if (dconfig.isValid()) {
        m_enabled = dconfig.enabled();
        m_currentTheme = dconfig.currentTheme();
        m_effectVolume = dconfig.effectVolume();
        if (!m_currentTheme.isEmpty()) m_player->setTheme(m_currentTheme);
        m_player->setVolume(m_effectVolume);
        qCInfo(memeOverlay) << "Loaded config: enabled=" << m_enabled
                            << "theme=" << m_currentTheme << "vol=" << m_effectVolume;
    }
}

EffectOverlay::~EffectOverlay() = default;

QScreen *EffectOverlay::findScreenForFile(const QString &filePath) const
{
    Q_UNUSED(filePath)
    return QGuiApplication::primaryScreen();
}

QPoint EffectOverlay::tryGetFileCoordinate(const QString &filePath) const
{
    if (filePath.isEmpty()) return QPoint(-1, -1);

    QDBusInterface iface("com.deepin.dde.desktop",
                         "/org/deepin/dde/desktop/canvas",
                         "org.deepin.dde.desktop.canvas",
                         QDBusConnection::sessionBus());
    if (!iface.isValid()) return QPoint(-1, -1);

    QDBusReply<QPoint> reply = iface.call("GetItemRect", filePath);
    if (reply.isValid()) return reply.value();
    return QPoint(-1, -1);
}

void EffectOverlay::triggerEffect(const QString &effectType, const QString &filePath)
{
    if (!m_enabled) return;

    QScreen *screen = findScreenForFile(filePath);
    if (!screen) {
        qCWarning(memeOverlay) << "No screen for file:" << filePath;
        return;
    }

    const QRect screenGeometry = screen->geometry();
    QPoint effectPos = tryGetFileCoordinate(filePath);
    if (effectPos.x() < 0 || effectPos.y() < 0) {
        effectPos = screenGeometry.center();
    }

    qCInfo(memeOverlay) << "Triggering:" << effectType << "at" << effectPos
                         << "file:" << filePath << "screen:" << screen->name();

    m_player->play(effectType, effectPos, screen, filePath);
    emit EffectTriggered(effectType, effectPos.x(), effectPos.y());
}

void EffectOverlay::SetEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        meme::MemeDConfig dconfig;
        dconfig.setEnabled(enabled);

        // 启用/禁用动态壁纸
        if (m_wallpaperManager) {
            if (enabled) m_wallpaperManager->enable();
            else m_wallpaperManager->disable();
        }
    }
}

bool EffectOverlay::GetEnabled()
{
    return m_enabled;
}

void EffectOverlay::ReloadThemes()
{
    if (m_player && m_player->themeResolver()) {
        m_player->themeResolver()->scanThemes();
        qCInfo(memeOverlay) << "Themes reloaded";
    }
}

void EffectOverlay::SetTheme(const QString &themeId)
{
    if (m_currentTheme != themeId) {
        m_currentTheme = themeId;
        m_player->setTheme(themeId);
        if (m_wallpaperManager) m_wallpaperManager->setTheme(themeId);
        meme::MemeDConfig dconfig;
        dconfig.setCurrentTheme(themeId);
        emit ThemeChanged(themeId);
    }
}

QString EffectOverlay::GetTheme() { return m_currentTheme; }

QStringList EffectOverlay::GetThemes()
{
    return m_player->themeResolver()->availableThemes();
}

void EffectOverlay::PreviewEffect(const QString &effectType)
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    m_player->play(effectType, screen->geometry().center(), screen, QString());
}

void EffectOverlay::SetEffectVolume(int volume)
{
    if (m_effectVolume != volume) {
        m_effectVolume = volume;
        m_player->setVolume(volume);
        meme::MemeDConfig dconfig;
        dconfig.setEffectVolume(volume);
    }
}
