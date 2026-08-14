// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEMECONFIG_H
#define MEMECONFIG_H

#include <QObject>

namespace meme {

// DConfig 键常量
struct MemeConfig {
    static constexpr const char *kOwner = "org.deepin.meme";
    static constexpr const char *kKeyEnabled = "enabled";
    static constexpr const char *kKeyCurrentTheme = "currentTheme";
    static constexpr const char *kKeyEffectVolume = "effectVolume";
};

} // namespace meme

#endif // MEMECONFIG_H
