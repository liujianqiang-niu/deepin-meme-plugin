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
    // 跨进程读取: DConfig::create(appId, name, subpath, parent)
    //   appId = schema owner (谁拥有这个配置)
    //   name  = schema 文件名(不含 .json)
    // 控制中心插件进程的 DSGApplication::id() 是 org.deepin.dde.control-center,
    // 必须用 create() 显式指定 appId=org.deepin.meme 才能找到正确的 schema。
    auto *cfg = DConfig::create(
        QStringLiteral("org.deepin.meme"),   // appId: schema owner
        QStringLiteral("org.deepin.meme"),   // name:  config file name
        QString(),                            // subpath
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
