// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEME_WALLPAPER_PLUGIN_H
#define MEME_WALLPAPER_PLUGIN_H

#include "global.h"
#include <dfm-framework/dpf.h>

namespace DD_MEME_WALLPAPER_NAMESPACE {
class WallpaperEngine;
class MemeWallpaperPlugin : public dpf::Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.deepin.plugin.desktop" FILE "meme_videowallpaper.json")
public:
    explicit MemeWallpaperPlugin(QObject *parent = nullptr);
    void initialize() override;
    bool start() override;
    void stop() override;
private:
    WallpaperEngine *engine = nullptr;
};
}
#endif
