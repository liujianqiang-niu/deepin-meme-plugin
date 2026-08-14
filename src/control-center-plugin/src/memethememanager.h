// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEMETHEMEMANAGER_H
#define MEMETHEMEMANAGER_H

#include <QObject>
#include <QStringList>
#include <optional>
#include <memory>
#include "thememanifest.h"

namespace meme {

// 主题包管理: 扫描 /usr/share/deepin-meme-themes/ 和用户目录
class MemeThemeManager : public QObject
{
    Q_OBJECT
public:
    explicit MemeThemeManager(QObject *parent = nullptr);

    // 扫描主题包目录
    void scanThemes();

    // 返回所有可用主题 ID 列表
    QStringList availableThemes() const;

    // 返回主题目录绝对路径
    QString themeDirectory(const QString &themeId) const;

    // 返回主题 manifest，不存在返回空
    std::optional<ThemeManifest> manifest(const QString &themeId) const;

private:
    // 搜索根目录列表（系统级 + 用户级）
    QStringList searchDirs() const;

    // themeId -> 绝对目录路径
    QHash<QString, QString> m_themeDirs;
    // themeId -> 解析后的 manifest
    QHash<QString, ThemeManifest> m_manifests;
};

} // namespace meme

#endif // MEMETHEMEMANAGER_H
