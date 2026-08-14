// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef THEMERESOLVER_H
#define THEMERESOLVER_H

#include <QObject>
#include <QStringList>
#include <QHash>
#include <optional>
#include "thememanifest.h"

namespace meme {

// 主题解析器: 扫描主题包目录，解析 manifest，提供查询
class ThemeResolver : public QObject
{
    Q_OBJECT
public:
    explicit ThemeResolver(QObject *parent = nullptr);

    void scanThemes();

    QStringList availableThemes() const;
    QString themeDirectory(const QString &themeId) const;
    std::optional<ThemeManifest> manifest(const QString &themeId) const;
    std::optional<EffectConfig> effectConfig(const QString &themeId, EffectType type) const;

private:
    QStringList searchDirs() const;
    QHash<QString, QString> m_themeDirs;
    QHash<QString, ThemeManifest> m_manifests;
};

} // namespace meme

#endif // THEMERESOLVER_H
