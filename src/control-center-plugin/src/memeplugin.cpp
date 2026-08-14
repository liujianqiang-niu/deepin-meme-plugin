// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "memeplugin.h"
#include "memethememanager.h"
#include "thememanifest.h"
#include "memeconfig.h"
#include "memedconfig.h"

#include "dccfactory.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QStandardPaths>

MemePlugin::MemePlugin(QObject *parent)
    : QObject(parent)
    , m_themeManager(new meme::MemeThemeManager(this))
{
    m_themeManager->scanThemes();

    // 从 DConfig 读取配置
    meme::MemeDConfig dconfig;
    if (dconfig.isValid()) {
        m_enabled = dconfig.enabled();
        m_effectVolume = dconfig.effectVolume();
        m_currentTheme = dconfig.currentTheme();
        // 如果配置的主题不存在,选择第一个可用主题
        const auto themes = m_themeManager->availableThemes();
        if (!themes.isEmpty() && !themes.contains(m_currentTheme)) {
            m_currentTheme = themes.first();
        }
    } else {
        // DConfig 不可用时的默认值
        m_enabled = false;
        m_effectVolume = 80;
        const auto themes = m_themeManager->availableThemes();
        if (!themes.isEmpty()) m_currentTheme = themes.first();
    }
}

MemePlugin::~MemePlugin() = default;

bool MemePlugin::enabled() const { return m_enabled; }

void MemePlugin::setEnabled(bool e)
{
    if (m_enabled != e) {
        m_enabled = e;
        emit enabledChanged(e);

        // 持久化到 DConfig
        meme::MemeDConfig dconfig;
        dconfig.setEnabled(e);

        auto bus = QDBusConnection::sessionBus();
        bus.call(QDBusMessage::createMethodCall(
            "org.deepin.meme.daemon",
            "/org/deepin/meme/daemon",
            "org.deepin.meme.daemon",
            "SetEnabled") << e);
    }
}

QString MemePlugin::currentTheme() const { return m_currentTheme; }

void MemePlugin::setCurrentTheme(const QString &id)
{
    if (m_currentTheme != id) {
        m_currentTheme = id;
        emit currentThemeChanged(id);

        // 持久化到 DConfig
        meme::MemeDConfig dconfig;
        dconfig.setCurrentTheme(id);

        auto bus = QDBusConnection::sessionBus();
        bus.call(QDBusMessage::createMethodCall(
            "org.deepin.meme.daemon",
            "/org/deepin/meme/daemon",
            "org.deepin.meme.daemon",
            "SetTheme") << id);
    }
}

QStringList MemePlugin::themeList() const
{
    return m_themeManager->availableThemes();
}

QVariantList MemePlugin::themeModel() const
{
    QVariantList model;
    for (const QString &id : m_themeManager->availableThemes()) {
        auto m = m_themeManager->manifest(id);
        if (!m) continue;
        QVariantMap entry;
        entry[QStringLiteral("id")] = id;
        entry[QStringLiteral("name")] = m->name;
        model.append(entry);
    }
    return model;
}

int MemePlugin::effectVolume() const { return m_effectVolume; }

void MemePlugin::setEffectVolume(int vol)
{
    if (m_effectVolume != vol) {
        m_effectVolume = vol;
        emit effectVolumeChanged(vol);

        meme::MemeDConfig dconfig;
        dconfig.setEffectVolume(vol);
    }
}

QString MemePlugin::previewVideoUrl(const QString &themeId) const
{
    const auto manifest = m_themeManager->manifest(themeId);
    if (!manifest) return {};
    const QString themeDir = m_themeManager->themeDirectory(themeId);
    return meme::resolvePath(themeDir, manifest->wallpaperPath);
}

QString MemePlugin::effectVideoUrl(const QString &themeId, const QString &effectType) const
{
    auto typeOpt = meme::stringToEffectType(effectType);
    if (!typeOpt) return {};
    auto manifest = m_themeManager->manifest(themeId);
    if (!manifest) return {};
    auto it = manifest->effects.find(*typeOpt);
    if (it == manifest->effects.end()) return {};
    const QString themeDir = m_themeManager->themeDirectory(themeId);
    return meme::resolvePath(themeDir, it->videoPath);
}

void MemePlugin::previewEffect(const QString &effectType)
{
    auto bus = QDBusConnection::sessionBus();
    bus.call(QDBusMessage::createMethodCall(
        "org.deepin.meme.daemon",
        "/org/deepin/meme/daemon",
        "org.deepin.meme.daemon",
        "PreviewEffect") << effectType);
}

DCC_FACTORY_CLASS(MemePlugin)
#include "memeplugin.moc"
