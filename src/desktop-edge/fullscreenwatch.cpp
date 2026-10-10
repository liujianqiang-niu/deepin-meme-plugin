// SPDX-License-Identifier: GPL-3.0-or-later
#include "fullscreenwatch.h"

#include <wayland-client.h>
#include "generated/wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"

namespace ddplugin_meme {

static const uint32_t kStateMaximized = ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED;
static const uint32_t kStateFullscreen = ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN;

static const wl_registry_listener kRegistryListener = {
    &FullscreenWatch::onRegistryGlobal,
    &FullscreenWatch::onRegistryGlobalRemove,
};

static const zwlr_foreign_toplevel_manager_v1_listener kManagerListener = {
    &FullscreenWatch::onManagerToplevel,
    &FullscreenWatch::onManagerFinished,
};

static const zwlr_foreign_toplevel_handle_v1_listener kHandleListener = {
    &FullscreenWatch::onToplevelTitle,
    &FullscreenWatch::onToplevelAppId,
    &FullscreenWatch::onToplevelOutputEnter,
    &FullscreenWatch::onToplevelOutputLeave,
    &FullscreenWatch::onToplevelState,
    &FullscreenWatch::onToplevelDone,
    &FullscreenWatch::onToplevelClosed,
    &FullscreenWatch::onToplevelParent,
};

static const wl_output_listener kOutputListener = {
    &FullscreenWatch::onOutputGeometry,
    &FullscreenWatch::onOutputMode,
    &FullscreenWatch::onOutputDone,
    &FullscreenWatch::onOutputScale,
    &FullscreenWatch::onOutputName,
    &FullscreenWatch::onOutputDescription,
};

FullscreenWatch::FullscreenWatch()
{
    display_ = wl_display_connect(nullptr);
    if (!display_)
        return;

    registry_ = wl_display_get_registry(display_);
    wl_registry_add_listener(registry_, &kRegistryListener, this);

    // 第一轮：拿到 globals 并绑定 manager / wl_output
    if (wl_display_roundtrip(display_) < 0) {
        managerFinished_ = true;
        return;
    }
    // 第二轮：初始的 toplevel 状态 + 输出名
    if (manager_ && wl_display_roundtrip(display_) < 0)
        managerFinished_ = true;
}

FullscreenWatch::~FullscreenWatch()
{
    for (auto h : toplevels_.keys())
        zwlr_foreign_toplevel_handle_v1_destroy(h);
    toplevels_.clear();
    if (manager_)
        zwlr_foreign_toplevel_manager_v1_destroy(manager_);
    if (registry_)
        wl_registry_destroy(registry_);
    if (display_)
        wl_display_disconnect(display_);
}

QSet<QString> FullscreenWatch::fullscreenOutputs()
{
    if (!display_ || !manager_ || managerFinished_)
        return {};
    if (wl_display_roundtrip(display_) < 0) {
        managerFinished_ = true;
        return {};
    }
    QSet<QString> out;
    for (auto it = toplevels_.constBegin(); it != toplevels_.constEnd(); ++it) {
        // 关键：按“当前”countMaximized_ 从原始状态重新判定，
        // 这样运行时切换 pauseOnMaximized 能立即生效（不受旧事件影响）
        bool covering = false;
        bool minimized = false;
        for (uint32_t v : it.value().rawStates) {
            if (v == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED)
                minimized = true;
            if (v == kStateFullscreen || (countMaximized_ && v == kStateMaximized))
                covering = true;
        }
        if (minimized)
            covering = false;   // 最小化的窗口不遮挡桌面
        if (!covering)
            continue;
        for (wl_output *o : it.value().outputs) {
            const QString n = outputNames_.value(o);
            if (!n.isEmpty())
                out.insert(n);
        }
    }
    return out;
}

void FullscreenWatch::onRegistryGlobal(void *data, wl_registry *reg, uint32_t name,
                                       const char *iface, uint32_t version)
{
    auto *self = static_cast<FullscreenWatch *>(data);
    const QString interface = QString::fromUtf8(iface);
    if (interface == QLatin1String("zwlr_foreign_toplevel_manager_v1")) {
        self->manager_ = static_cast<zwlr_foreign_toplevel_manager_v1 *>(
            wl_registry_bind(reg, name, &zwlr_foreign_toplevel_manager_v1_interface,
                             qMin<uint32_t>(version, 3)));
        if (self->manager_)
            zwlr_foreign_toplevel_manager_v1_add_listener(self->manager_, &kManagerListener, self);
    } else if (interface == QLatin1String("wl_output")) {
        auto *o = static_cast<wl_output *>(
            wl_registry_bind(reg, name, &wl_output_interface, qMin<uint32_t>(version, 4)));
        if (o)
            wl_output_add_listener(o, &kOutputListener, self);
    }
}

void FullscreenWatch::onRegistryGlobalRemove(void *, wl_registry *, uint32_t) {}

void FullscreenWatch::onManagerToplevel(void *data, zwlr_foreign_toplevel_manager_v1 *,
                                        zwlr_foreign_toplevel_handle_v1 *h)
{
    auto *self = static_cast<FullscreenWatch *>(data);
    if (!h)
        return;
    zwlr_foreign_toplevel_handle_v1_add_listener(h, &kHandleListener, self);
    self->toplevels_.insert(h, Toplevel{});
}

void FullscreenWatch::onManagerFinished(void *data, zwlr_foreign_toplevel_manager_v1 *)
{
    static_cast<FullscreenWatch *>(data)->managerFinished_ = true;
}

void FullscreenWatch::onToplevelTitle(void *data, zwlr_foreign_toplevel_handle_v1 *h, const char *t)
{
    static_cast<FullscreenWatch *>(data)->toplevels_[h].title = QString::fromUtf8(t ? t : "");
}
void FullscreenWatch::onToplevelAppId(void *data, zwlr_foreign_toplevel_handle_v1 *h, const char *a)
{
    static_cast<FullscreenWatch *>(data)->toplevels_[h].app = QString::fromUtf8(a ? a : "");
}
void FullscreenWatch::onToplevelDone(void *, zwlr_foreign_toplevel_handle_v1 *) {}
void FullscreenWatch::onToplevelParent(void *, zwlr_foreign_toplevel_handle_v1 *, zwlr_foreign_toplevel_handle_v1 *) {}

void FullscreenWatch::onToplevelOutputEnter(void *data, zwlr_foreign_toplevel_handle_v1 *h, wl_output *o)
{
    auto *self = static_cast<FullscreenWatch *>(data);
    self->toplevels_[h].outputs.insert(o);
    self->notifyChange();
}

void FullscreenWatch::onToplevelOutputLeave(void *data, zwlr_foreign_toplevel_handle_v1 *h, wl_output *o)
{
    auto *self = static_cast<FullscreenWatch *>(data);
    self->toplevels_[h].outputs.remove(o);
    self->notifyChange();
}

void FullscreenWatch::onToplevelState(void *data, zwlr_foreign_toplevel_handle_v1 *h, wl_array *state)
{
    auto *self = static_cast<FullscreenWatch *>(data);
    auto &raw = self->toplevels_[h].rawStates;
    raw.clear();
    if (state && state->data) {
        const auto count = state->size / sizeof(uint32_t);
        auto *vals = static_cast<uint32_t *>(state->data);
        for (size_t i = 0; i < count; ++i)
            raw.push_back(vals[i]);
    }
    // “是否遮挡”在 fullscreenOutputs() 里按当前 countMaximized_ 现算，
    // 这样运行时切换开关能立即生效。
    self->notifyChange();
}

void FullscreenWatch::notifyChange()
{
    if (onChange_)
        onChange_();
}

QString FullscreenWatch::debugDump()
{
    if (!display_ || !manager_)
        return QStringLiteral("<no manager>");
    wl_display_roundtrip(display_);
    QString s;
    for (auto it = toplevels_.constBegin(); it != toplevels_.constEnd(); ++it) {
        QStringList st;
        for (uint32_t v : it.value().rawStates)
            st << QString::number(v);
        QStringList outs;
        for (wl_output *o : it.value().outputs)
            outs << outputNames_.value(o, QStringLiteral("?"));
        s += QStringLiteral("app=[%1] title=[%2] states=[%3] outs=[%4]\n")
                 .arg(it.value().app, it.value().title, st.join(','), outs.join(','));
    }
    return s;
}

void FullscreenWatch::onToplevelClosed(void *data, zwlr_foreign_toplevel_handle_v1 *h)
{
    auto *self = static_cast<FullscreenWatch *>(data);
    self->toplevels_.remove(h);
    zwlr_foreign_toplevel_handle_v1_destroy(h);
    self->notifyChange();
}

void FullscreenWatch::onOutputGeometry(void *, wl_output *, int32_t, int32_t, int32_t, int32_t,
                                       int32_t, const char *, const char *, int32_t) {}
void FullscreenWatch::onOutputMode(void *, wl_output *, uint32_t, int32_t, int32_t, int32_t) {}
void FullscreenWatch::onOutputDone(void *, wl_output *) {}
void FullscreenWatch::onOutputScale(void *, wl_output *, int32_t) {}
void FullscreenWatch::onOutputDescription(void *, wl_output *, const char *) {}

void FullscreenWatch::onOutputName(void *data, wl_output *o, const char *name)
{
    auto *self = static_cast<FullscreenWatch *>(data);
    if (name)
        self->outputNames_.insert(o, QString::fromUtf8(name));
}

} // namespace ddplugin_meme
