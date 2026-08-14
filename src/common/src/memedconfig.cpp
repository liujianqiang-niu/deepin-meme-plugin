// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "memedconfig.h"

#include <dconfig.h>

using DConfig = Dtk::Core::DConfig;

namespace meme {

MemeDConfig::MemeDConfig(QObject *parent)
    : QObject(parent)
{
    auto *cfg = DConfig::create(
        QStringLiteral("org.deepin.meme"),
        QStringLiteral("org.deepin.meme"),
        QString(),
        this);
    m_dconfig = cfg;

    if (cfg) {
        connect(cfg, &DConfig::valueChanged, this, [this](const QString &key) {
            emit changed(key);
        });
    }
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
    return cfg->value(QStringLiteral("enabled"), false).toBool();
}

void MemeDConfig::setEnabled(bool e)
{
    auto *cfg = static_cast<DConfig *>(m_dconfig);
    if (!cfg || !cfg->isValid()) return;
    cfg->setValue(QStringLiteral("enabled"), e);
}

QString MemeDConfig::currentVideo() const
{
    auto *cfg = static_cast<DConfig *>(m_dconfig);
    if (!cfg || !cfg->isValid()) return {};
    return cfg->value(QStringLiteral("currentVideo"), QString()).toString();
}

void MemeDConfig::setCurrentVideo(const QString &path)
{
    auto *cfg = static_cast<DConfig *>(m_dconfig);
    if (!cfg || !cfg->isValid()) return;
    cfg->setValue(QStringLiteral("currentVideo"), path);
}

} // namespace meme
