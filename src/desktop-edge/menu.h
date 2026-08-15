// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEME_WALLPAPER_MENU_SCENE_H
#define MEME_WALLPAPER_MENU_SCENE_H

#include <dfm-base/interfaces/abstractmenuscene.h>
#include <dfm-base/interfaces/abstractscenecreator.h>

#include <QMap>

namespace ddplugin_meme {

namespace ActionID {
inline constexpr char kMemeWallpaper[] = "meme-wallpaper";
inline constexpr char kMemeWallpaperSettings[] = "meme-wallpaper-settings";
}

class MemeWallpaperMenuCreator : public DFMBASE_NAMESPACE::AbstractSceneCreator
{
    Q_OBJECT
public:
    static QString name()
    {
        return "MemeWallpaperMenu";
    }
    DFMBASE_NAMESPACE::AbstractMenuScene *create() override;
};

class MemeWallpaperMenuScene : public DFMBASE_NAMESPACE::AbstractMenuScene
{
    Q_OBJECT
public:
    explicit MemeWallpaperMenuScene(QObject *parent = nullptr);
    QString name() const override;
    bool initialize(const QVariantHash &params) override;
    AbstractMenuScene *scene(QAction *action) const override;
    bool create(QMenu *parent) override;
    void updateState(QMenu *parent) override;
    bool triggered(QAction *action) override;
private:
    bool turnOn = false;
    bool onDesktop = false;
    bool isEmptyArea = false;
    QMap<QString, QAction *> predicateAction;
    QMap<QString, QString> predicateName;
};

}

#endif // MEME_WALLPAPER_MENU_SCENE_H
