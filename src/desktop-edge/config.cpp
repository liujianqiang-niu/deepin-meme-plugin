// SPDX-License-Identifier: GPL-3.0-or-later
#include "config.h"

#include <DConfig>

using namespace ddplugin_meme;

static const char *kAppId = "org.deepin.meme";

MemeConfig::MemeConfig(QObject *parent)
    : QObject(parent)
{
    m_cfg = DConfig::create(kAppId, kAppId, QString(), this);
    if (m_cfg && m_cfg->isValid()) {
        connect(m_cfg, &DConfig::valueChanged, this, [this](const QString &key) {
            Q_UNUSED(key)
            emit configChanged();
        });
    } else {
        qWarning() << "[meme-wallpaper] DConfig init failed for" << kAppId;
    }
}

MemeConfig::~MemeConfig()
{
    // m_cfg 是 this 的子对象，Qt 自动释放
}

bool MemeConfig::isValid() const
{
    return m_cfg && m_cfg->isValid();
}

// --- 读取 ---

bool MemeConfig::enabled() const
{
    if (!m_cfg)
        return false;
    return m_cfg->value("enabled", false).toBool();
}

QString MemeConfig::currentVideo() const
{
    if (!m_cfg)
        return {};
    return m_cfg->value("currentVideo", QString()).toString();
}

DecodeMode MemeConfig::decodeMode() const
{
    if (!m_cfg)
        return DecodeMode::Auto;
    return decodeModeFromString(m_cfg->value("decodeMode", "software").toString());
}

FillMode MemeConfig::fillMode() const
{
    if (!m_cfg)
        return FillMode::Fill;
    return fillModeFromString(m_cfg->value("fillMode", "fill").toString());
}

// --- 写入 ---

void MemeConfig::setEnabled(bool e)
{
    if (m_cfg)
        m_cfg->setValue("enabled", e);
}

void MemeConfig::setCurrentVideo(const QString &path)
{
    if (m_cfg)
        m_cfg->setValue("currentVideo", path);
}

void MemeConfig::setDecodeMode(DecodeMode mode)
{
    if (m_cfg)
        m_cfg->setValue("decodeMode", decodeModeToString(mode));
}

void MemeConfig::setFillMode(FillMode mode)
{
    if (m_cfg)
        m_cfg->setValue("fillMode", fillModeToString(mode));
}

// --- 字符串 ↔ 枚举转换 ---

DecodeMode MemeConfig::decodeModeFromString(const QString &s)
{
    const QString v = s.toLower().trimmed();
    if (v == "cuda")
        return DecodeMode::Cuda;
    if (v == "vaapi")
        return DecodeMode::Vaapi;
    if (v == "software")
        return DecodeMode::Software;
    if (v == "auto")
        return DecodeMode::Auto;
    return DecodeMode::Software;  // 默认软解，最兼容
}

QString MemeConfig::decodeModeToString(DecodeMode m)
{
    switch (m) {
    case DecodeMode::Cuda:     return "cuda";
    case DecodeMode::Vaapi:    return "vaapi";
    case DecodeMode::Software: return "software";
    case DecodeMode::Auto:     return "auto";
    }
    return "software";
}

FillMode MemeConfig::fillModeFromString(const QString &s)
{
    const QString v = s.toLower().trimmed();
    if (v == "fit")
        return FillMode::Fit;
    if (v == "stretch")
        return FillMode::Stretch;
    if (v == "center")
        return FillMode::Center;
    if (v == "tile")
        return FillMode::Tile;
    return FillMode::Fill;  // 默认铺满
}

QString MemeConfig::fillModeToString(FillMode m)
{
    switch (m) {
    case FillMode::Fill:    return "fill";
    case FillMode::Fit:     return "fit";
    case FillMode::Stretch: return "stretch";
    case FillMode::Center:  return "center";
    case FillMode::Tile:    return "tile";
    }
    return "fill";
}
