// SPDX-License-Identifier: GPL-3.0-or-later
#include "global.h"
#include "menu.h"

#include <dfm-base/dfm_menu_defines.h>

#include <DConfig>

#include <QVariantHash>
#include <QMenu>
#include <QApplication>
#include <QTimer>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QDebug>

using namespace ddplugin_meme;
DFMBASE_USE_NAMESPACE

DCORE_USE_NAMESPACE

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
    // 从 DConfig 读取当前开关状态（用持久 DConfig 避免局部变量提前析构导致写入丢失）
    auto *cfg = DConfig::create("org.deepin.meme", "org.deepin.meme", QString(), qApp);
    if (cfg && cfg->isValid())
        turnOn = cfg->value("enabled", false).toBool();
    // cfg 是 qApp 子对象，不手动 delete，随进程退出释放

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
    auto *cfg = DConfig::create("org.deepin.meme", "org.deepin.meme", QString(), qApp);
    if (cfg && cfg->isValid())
        toggle->setChecked(cfg->value("enabled", false).toBool());

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
    qInfo() << "[meme-wallpaper] menu triggered, action=" << action;
    if (!action || !predicateAction.values().contains(action)) {
        qWarning() << "[meme-wallpaper] menu triggered: action not in predicateAction, fallback";
        return AbstractMenuScene::triggered(action);
    }

    const auto actionId = action->property(ActionPropertyKey::kActionID).toString();
    qInfo() << "[meme-wallpaper] menu triggered: actionId=" << actionId;

    if (actionId == ActionID::kMemeWallpaper) {
        // 写入 DConfig：engine 监听 configChanged 自动启停
        // 用 QTimer 延迟到主线程事件循环，确保 DConfig 对象存活到写入完成
        const bool checked = action->isChecked();
        QTimer::singleShot(0, qApp, [checked]() {
            auto *cfg = DConfig::create("org.deepin.meme", "org.deepin.meme", QString(), qApp);
            if (cfg && cfg->isValid()) {
                cfg->setValue("enabled", checked);
                qInfo() << "[meme-wallpaper] DConfig set enabled=" << checked;
            } else {
                qWarning() << "[meme-wallpaper] DConfig invalid, cannot set enabled";
            }
        });
        return true;
    }
    if (actionId == ActionID::kMemeWallpaperSettings) {
        // D-Bus 调用控制中心打开壁纸设置页
        // 正确服务名: org.deepin.dde.ControlCenter1 (带 1 后缀)
        QTimer::singleShot(0, qApp, []() {
            QDBusInterface cc(QStringLiteral("org.deepin.dde.ControlCenter1"),
                              QStringLiteral("/org/deepin/dde/ControlCenter1"),
                              QStringLiteral("org.deepin.dde.ControlCenter1"),
                              QDBusConnection::sessionBus());
            if (cc.isValid()) {
                cc.asyncCall(QStringLiteral("ShowPage"),
                             QStringLiteral("personalization/meme"));
                qInfo() << "[meme-wallpaper] ControlCenter ShowPage sent";
            } else {
                qWarning() << "[meme-wallpaper] ControlCenter1 D-Bus interface not available";
            }
        });
        return true;
    }
    return AbstractMenuScene::triggered(action);
}
