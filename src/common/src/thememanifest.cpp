// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "thememanifest.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStandardPaths>

namespace meme {

QString effectTypeToString(EffectType type)
{
    switch (type) {
    case EffectType::Delete: return QStringLiteral("delete");
    case EffectType::Create: return QStringLiteral("create");
    case EffectType::Rename: return QStringLiteral("rename");
    case EffectType::Move:   return QStringLiteral("move");
    case EffectType::Copy:   return QStringLiteral("copy");
    }
    return {};
}

std::optional<EffectType> stringToEffectType(const QString &str)
{
    if (str == "delete") return EffectType::Delete;
    if (str == "create") return EffectType::Create;
    if (str == "rename") return EffectType::Rename;
    if (str == "move")   return EffectType::Move;
    if (str == "copy")   return EffectType::Copy;
    return std::nullopt;
}

static EffectConfig parseEffect(const QJsonObject &obj)
{
    EffectConfig cfg;
    cfg.videoPath = obj.value("video").toString();
    cfg.audioPath = obj.value("audio").toString();
    cfg.anchor = obj.value("anchor").toString("target");
    cfg.scale = obj.value("scale").toDouble(1.0);
    cfg.duration = static_cast<int>(obj.value("duration").toDouble(3000.0));
    return cfg;
}

std::optional<ThemeManifest> parseManifest(const QString &themeDir)
{
    const QString manifestPath = QDir(themeDir).absoluteFilePath("manifest.json");
    QFile f(manifestPath);
    if (!f.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }

    const QByteArray data = f.readAll();
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return std::nullopt;
    }

    const QJsonObject root = doc.object();
    ThemeManifest m;
    m.id = root.value("id").toString();
    m.name = root.value("name").toString();
    m.description = root.value("description").toString();
    m.version = root.value("version").toString();
    m.author = root.value("author").toString();
    m.wallpaperPath = root.value("wallpaper").toString();
    m.thumbnailPath = root.value("thumbnail").toString();

    const QJsonObject effects = root.value("effects").toObject();
    for (auto it = effects.begin(); it != effects.end(); ++it) {
        const auto type = stringToEffectType(it.key());
        if (!type) continue;
        if (!it.value().isObject()) continue;
        m.effects[*type] = parseEffect(it.value().toObject());
    }

    if (m.id.isEmpty()) {
        return std::nullopt;
    }
    return m;
}

QString resolvePath(const QString &themeDir, const QString &relativePath)
{
    if (relativePath.isEmpty()) return {};
    if (QDir::isAbsolutePath(relativePath)) return relativePath;
    return QDir(themeDir).absoluteFilePath(relativePath);
}

} // namespace meme
