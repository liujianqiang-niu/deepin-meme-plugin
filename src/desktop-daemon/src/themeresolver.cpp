// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "themeresolver.h"

#include <QDir>
#include <QStandardPaths>

namespace meme {

ThemeResolver::ThemeResolver(QObject *parent)
    : QObject(parent)
{
}

QStringList ThemeResolver::searchDirs() const
{
    return {
        QStringLiteral("/usr/share/deepin-meme-themes"),
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + QStringLiteral("/deepin-meme-themes"),
    };
}

void ThemeResolver::scanThemes()
{
    m_themeDirs.clear();
    m_manifests.clear();

    for (const QString &baseDir : searchDirs()) {
        QDir dir(baseDir);
        if (!dir.exists()) continue;

        const auto entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &name : entries) {
            const QString themeDir = dir.absoluteFilePath(name);
            auto manifest = parseManifest(themeDir);
            if (!manifest) continue;
            if (m_themeDirs.contains(manifest->id)) continue;

            m_themeDirs.insert(manifest->id, themeDir);
            m_manifests.insert(manifest->id, *manifest);
        }
    }
}

QStringList ThemeResolver::availableThemes() const
{
    return m_themeDirs.keys();
}

QString ThemeResolver::themeDirectory(const QString &themeId) const
{
    return m_themeDirs.value(themeId);
}

std::optional<ThemeManifest> ThemeResolver::manifest(const QString &themeId) const
{
    auto it = m_manifests.find(themeId);
    if (it == m_manifests.end()) return std::nullopt;
    return it.value();
}

std::optional<EffectConfig> ThemeResolver::effectConfig(const QString &themeId, EffectType type) const
{
    auto m = manifest(themeId);
    if (!m) return std::nullopt;
    auto it = m->effects.find(type);
    if (it == m->effects.end()) return std::nullopt;
    return it.value();
}

} // namespace meme
