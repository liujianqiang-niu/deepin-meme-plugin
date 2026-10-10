// SPDX-License-Identifier: GPL-3.0-or-later
#include "config.h"

#include <DConfig>

DCORE_USE_NAMESPACE

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
    return decodeModeFromString(m_cfg->value("decodeMode", "auto").toString());
}

FillMode MemeConfig::fillMode() const
{
    if (!m_cfg)
        return FillMode::Fill;
    return fillModeFromString(m_cfg->value("fillMode", "fill").toString());
}

bool MemeConfig::pauseOnMaximized() const
{
    if (!m_cfg)
        return false;
    return m_cfg->value("pauseOnMaximized", false).toBool();
}

double MemeConfig::fpsCap() const
{
    if (!m_cfg)
        return 0.0;
    return m_cfg->value("fpsCap", 0.0).toDouble();
}

int MemeConfig::renderWidth() const
{
    if (!m_cfg)
        return 0;
    return m_cfg->value("renderWidth", 0).toInt();
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

void MemeConfig::setPauseOnMaximized(bool b)
{
    if (m_cfg)
        m_cfg->setValue("pauseOnMaximized", b);
}

void MemeConfig::setFpsCap(double fps)
{
    if (m_cfg)
        m_cfg->setValue("fpsCap", fps);
}

void MemeConfig::setRenderWidth(int w)
{
    if (m_cfg)
        m_cfg->setValue("renderWidth", w);
}

// --- 字符串 ↔ 枚举转换 ---

DecodeMode MemeConfig::decodeModeFromString(const QString &s)
{
    const QString v = s.toLower().trimmed();
    if (v == "cuda")
        return DecodeMode::Cuda;
    if (v == "vaapi")
        return DecodeMode::Vaapi;
    if (v == "vulkan")
        return DecodeMode::Vulkan;
    if (v == "software")
        return DecodeMode::Software;
    if (v == "auto")
        return DecodeMode::Auto;
    return DecodeMode::Auto;  // 未知值 → 自动
}

QString MemeConfig::decodeModeToString(DecodeMode m)
{
    switch (m) {
    case DecodeMode::Cuda:     return "cuda";
    case DecodeMode::Vaapi:    return "vaapi";
    case DecodeMode::Vulkan:   return "vulkan";
    case DecodeMode::Software: return "software";
    case DecodeMode::Auto:     return "auto";
    }
    return "auto";
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
