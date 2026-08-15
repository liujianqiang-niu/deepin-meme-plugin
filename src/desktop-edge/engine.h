// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEME_WALLPAPER_ENGINE_H
#define MEME_WALLPAPER_ENGINE_H

#include "global.h"

#include <QObject>
#include <QImage>
#include <QUrl>

namespace ddplugin_meme {

class WallpaperEnginePrivate;
class WallpaperEngine : public QObject
{
    Q_OBJECT
    friend class WallpaperEnginePrivate;
public:
    explicit WallpaperEngine(QObject *parent = nullptr);
    ~WallpaperEngine() override;
    bool init();
    void turnOn(bool build = true);
    void turnOff();

public slots:
    void refreshSource();
    void build();
    void onDetachWindows();
    void geometryChanged();
    void play();
    void show();

private slots:
    bool registerMenu();
    void checkResouce();
    void catchImage(const QImage &img);
    void onOptionsChanged();
    void onSessionLockSignal();
    void onScreenSaverActiveChanged(bool active);

private:
    WallpaperEnginePrivate *d;
};

}

#endif // MEME_WALLPAPER_ENGINE_H
