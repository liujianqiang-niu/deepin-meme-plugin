// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef THEMEMANIFEST_H
#define THEMEMANIFEST_H

#include <QJsonObject>
#include <QString>
#include <QHash>
#include <optional>

namespace meme {

// 特效触发的事件类型
enum class EffectType {
    Delete,   // 删除/移到回收站
    Create,   // 新建文件/目录
    Rename,   // 重命名
    Move,     // 移动
    Copy      // 复制
};

// 单个特效的配置
struct EffectConfig {
    QString videoPath;   // 特效视频文件相对路径
    QString audioPath;   // 音效文件相对路径
    QString anchor;      // 锚点: "target" | "screen-center" | "corner-tl"
    double scale = 1.0;   // 缩放系数
    int duration = 3000;  // 持续时长(ms)
};

// 主题包 manifest 数据
struct ThemeManifest {
    QString id;
    QString name;
    QString description;
    QString version;
    QString author;
    QString wallpaperPath;      // 循环壁纸视频相对路径
    QString thumbnailPath;       // 缩略图相对路径
    QHash<EffectType, EffectConfig> effects;
};

// 工具函数
QString effectTypeToString(EffectType type);
std::optional<EffectType> stringToEffectType(const QString &str);

// 解析 manifest.json，返回解析后的 ThemeManifest。失败返回空 optional
std::optional<ThemeManifest> parseManifest(const QString &themeDir);

// 根据 themeDir 和 manifest 中的相对路径，返回绝对路径
QString resolvePath(const QString &themeDir, const QString &relativePath);

} // namespace meme

#endif // THEMEMANIFEST_H
