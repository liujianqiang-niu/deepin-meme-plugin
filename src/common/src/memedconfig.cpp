// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "memedconfig.h"
#include "memeconfig.h"

#include <dconfig.h>

using DConfig = Dtk::Core::DConfig;

namespace meme {

MemeDConfig::MemeDConfig(QObject *parent)
    : QObject(parent)
{
    // DConfig 构造: DConfig(name, subpath, parent)
    // name 对应 DConfig schema 文件名(不含 .json)
    auto *cfg = new DConfig(QStringLiteral("org.deepin.meme"), QString(), this);
    m_dconfig = cfg;

    connect(cfg, &DConfig::valueChanged, this, [this](const QString &key) {
        emit changed(key);
    });
}

MemeDConfig::~MemeDConfig() = default;

bool MemeDConfig::isValid() const
{
    auto *cfg = static_cast<DConfig *>(m_dconfig);
    return cfg && cfg->isValid();
}

bool MemeDConfig::enabled() const
{
    auto *cfg = static_cast<DConfig *>(m_dconfig);
    if (!cfg || !cfg->isValid()) return false;
    return cfg->value(MemeConfig::kKeyEnabled, false).toBool();
}

void MemeDConfig::setEnabled(bool e)
{
    auto *cfg = static_cast<DConfig *>(m_dconfig);
    if (!cfg || !cfg->isValid()) return;
    cfg->setValue(MemeConfig::kKeyEnabled, e);
}

QString MemeDConfig::currentTheme() const
{
    auto *cfg = static_cast<DConfig *>(m_dconfig);
    if (!cfg || !cfg->isValid()) return QStringLiteral("example");
    return cfg->value(MemeConfig::kKeyCurrentTheme, QStringLiteral("example")).toString();
}

void MemeDConfig::setCurrentTheme(const QString &id)
{
    auto *cfg = static_cast<DConfig *>(m_dconfig);
    if (!cfg || !cfg->isValid()) return;
    cfg->setValue(MemeConfig::kKeyCurrentTheme, id);
}

int MemeDConfig::effectVolume() const
{
    auto *cfg = static_cast<DConfig *>(m_dconfig);
    if (!cfg || !cfg->isValid()) return 80;
    return cfg->value(MemeConfig::kKeyEffectVolume, 80).toInt();
}

void MemeDConfig::setEffectVolume(int vol)
{
    auto *cfg = static_cast<DConfig *>(m_dconfig);
    if (!cfg || !cfg->isValid()) return;
    cfg->setValue(MemeConfig::kKeyEffectVolume, vol);
}

} // namespace meme
