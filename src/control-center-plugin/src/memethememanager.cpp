// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "memethememanager.h"

#include <QDir>
#include <QStandardPaths>

namespace meme {

MemeThemeManager::MemeThemeManager(QObject *parent)
    : QObject(parent)
{
}

QStringList MemeThemeManager::searchDirs() const
{
    return {
        QStringLiteral("/usr/share/deepin-meme-themes"),
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + QStringLiteral("/deepin-meme-themes"),
    };
}

void MemeThemeManager::scanThemes()
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

            const QString id = manifest->id;
            if (m_themeDirs.contains(id)) continue; // 系统级优先
            m_themeDirs.insert(id, themeDir);
            m_manifests.insert(id, *manifest);
        }
    }
}

QStringList MemeThemeManager::availableThemes() const
{
    return m_themeDirs.keys();
}

QString MemeThemeManager::themeDirectory(const QString &themeId) const
{
    return m_themeDirs.value(themeId);
}

std::optional<ThemeManifest> MemeThemeManager::manifest(const QString &themeId) const
{
    auto it = m_manifests.find(themeId);
    if (it == m_manifests.end()) return std::nullopt;
    return it.value();
}

} // namespace meme
