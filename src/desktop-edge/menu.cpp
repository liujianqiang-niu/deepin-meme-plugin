// SPDX-License-Identifier: GPL-3.0-or-later
#include "global.h"
#include "menu.h"
#include "config.h"

#include <dfm-base/dfm_menu_defines.h>

#include <QVariantHash>
#include <QMenu>
#include <QApplication>
#include <QTimer>
#include <QDBusInterface>
#include <QDebug>

using namespace ddplugin_meme;
DFMBASE_USE_NAMESPACE

AbstractMenuScene *MemeWallpaperMenuCreator::create()
{
    return new MemeWallpaperMenuScene();
}

MemeWallpaperMenuScene::MemeWallpaperMenuScene(QObject *parent)
    : AbstractMenuScene(parent)
{
    predicateName[ActionID::kMemeWallpaper] = tr("动态壁纸");
    predicateName[ActionID::kMemeWallpaperSettings] = tr("动态壁纸设置…");
}

QString MemeWallpaperMenuScene::name() const
{
    return MemeWallpaperMenuCreator::name();
}

bool MemeWallpaperMenuScene::initialize(const QVariantHash &params)
{
    // 从 DConfig 读取当前开关状态
    MemeConfig cfg;
    turnOn = cfg.enabled();
    isEmptyArea = params.value(MenuParamKey::kIsEmptyArea).toBool();
    onDesktop = params.value(MenuParamKey::kOnDesktop).toBool();
    return isEmptyArea && onDesktop;
}

AbstractMenuScene *MemeWallpaperMenuScene::scene(QAction *action) const
{
    if (!action)
        return nullptr;
    if (predicateAction.values().contains(action))
        return const_cast<MemeWallpaperMenuScene *>(this);
    return AbstractMenuScene::scene(action);
}

bool MemeWallpaperMenuScene::create(QMenu *parent)
{
    Q_UNUSED(parent)
    auto *toggle = new QAction(predicateName.value(ActionID::kMemeWallpaper), this);
    toggle->setProperty(ActionPropertyKey::kActionID, QString(ActionID::kMemeWallpaper));
    toggle->setCheckable(true);
    toggle->setChecked(turnOn);
    predicateAction[ActionID::kMemeWallpaper] = toggle;

    auto *settings = new QAction(predicateName.value(ActionID::kMemeWallpaperSettings), this);
    settings->setProperty(ActionPropertyKey::kActionID, QString(ActionID::kMemeWallpaperSettings));
    predicateAction[ActionID::kMemeWallpaperSettings] = settings;
    return true;
}

void MemeWallpaperMenuScene::updateState(QMenu *parent)
{
    if (!parent)
        return;

    auto *toggle = predicateAction.value(ActionID::kMemeWallpaper);
    auto *settings = predicateAction.value(ActionID::kMemeWallpaperSettings);
    if (!toggle || !settings)
        return;

    // 从 DConfig 实时读取
    MemeConfig cfg;
    toggle->setChecked(cfg.enabled());

    auto actions = parent->actions();
    auto actionIter = std::find_if(actions.begin(), actions.end(), [](const QAction *ac) {
        return ac && ac->property(ActionPropertyKey::kActionID).toString() == QStringLiteral("wallpaper-settings");
    });

    if (actionIter != actions.end()) {
        QAction *indexAction = *actionIter;
        parent->insertAction(indexAction, settings);
        parent->insertAction(settings, toggle);
    } else {
        parent->addAction(toggle);
        parent->addAction(settings);
    }
    AbstractMenuScene::updateState(parent);
}

bool MemeWallpaperMenuScene::triggered(QAction *action)
{
    if (!action || !predicateAction.values().contains(action))
        return AbstractMenuScene::triggered(action);

    const auto actionId = action->property(ActionPropertyKey::kActionID).toString();
    if (actionId == ActionID::kMemeWallpaper) {
        // 写入 DConfig：engine 监听 configChanged 自动启停
        MemeConfig cfg;
        cfg.setEnabled(action->isChecked());
        return true;
    }
    if (actionId == ActionID::kMemeWallpaperSettings) {
        // D-Bus 调用控制中心打开壁纸设置页
        QTimer::singleShot(0, qApp, []() {
            QDBusInterface cc(QStringLiteral("org.deepin.dde.ControlCenter"),
                              QStringLiteral("/org/deepin/dde/ControlCenter"),
                              QStringLiteral("org.deepin.dde.ControlCenter"),
                              QDBusConnection::sessionBus());
            if (cc.isValid()) {
                cc.asyncCall(QStringLiteral("ShowPage"),
                             QStringLiteral("personalization/meme"));
            } else {
                qWarning() << "[meme-wallpaper] ControlCenter D-Bus interface not available";
            }
        });
        return true;
    }
    return AbstractMenuScene::triggered(action);
}
