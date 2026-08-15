// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEME_EVENT_HELPER_H
#define MEME_EVENT_HELPER_H

#include <dfm-base/interfaces/screen/abstractscreenproxy.h>
#include <dfm-framework/dpf.h>

#define CanvasCorePush(topic) dpfSlotChannel->push("ddplugin_core", QT_STRINGIFY2(topic))
#define CanvasCorePush2(topic, args...) dpfSlotChannel->push("ddplugin_core", QT_STRINGIFY2(topic), ##args)

namespace ddplugin_meme_util {
static inline QList<QWidget *> desktopFrameRootWindows() {
    const QVariant &ret = CanvasCorePush(slot_DesktopFrame_RootWindows);
    return ret.value<QList<QWidget *>>();
}
}
#endif
