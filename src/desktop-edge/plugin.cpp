// SPDX-License-Identifier: GPL-3.0-or-later
#include "plugin.h"
#include "engine.h"
#include <QDebug>

DD_MEME_WALLPAPER_USE_NAMESPACE

DD_MEME_WALLPAPER_BEGIN_NAMESPACE
DFM_LOG_REGISTER_CATEGORY(DD_MEME_WALLPAPER_NAMESPACE)
DD_MEME_WALLPAPER_END_NAMESPACE

MemeWallpaperPlugin::MemeWallpaperPlugin(QObject *parent) : Plugin() { Q_UNUSED(parent) }

void MemeWallpaperPlugin::initialize() {}

bool MemeWallpaperPlugin::start()
{
    qWarning() << "[meme-wallpaper] plugin start() called";
    engine = new WallpaperEngine();
    bool ok = engine->init();
    qWarning() << "[meme-wallpaper] engine init() returned" << ok;
    return ok;
}

void MemeWallpaperPlugin::stop()
{
    delete engine;
    engine = nullptr;
}
