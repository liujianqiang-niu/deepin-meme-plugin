// SPDX-License-Identifier: GPL-3.0-or-later
// 通过 wlr-foreign-toplevel-management 协议，检测哪些输出(显示器)当前有“全屏窗口”。
// 用于 meme 动态壁纸：全屏窗口盖住桌面时暂停解码，省掉无意义的 CPU/GPU 开销。
// 思路与 Fresco 的 fullscreen.rs 一致（只认 fullscreen，不认 maximized）。
#ifndef MEME_FULLSCREENWATCH_H
#define MEME_FULLSCREENWATCH_H

#include <QString>
#include <QSet>
#include <QHash>
#include <functional>

struct wl_display;
struct wl_registry;
struct wl_output;
struct wl_array;
struct zwlr_foreign_toplevel_manager_v1;
struct zwlr_foreign_toplevel_handle_v1;

namespace ddplugin_meme {

class FullscreenWatch
{
public:
    FullscreenWatch();
    ~FullscreenWatch();

    bool valid() const { return display_ != nullptr && manager_ != nullptr; }

    // 轮询 compositor，返回“当前有全屏窗口”的输出连接器名集合（如 "HDMI-A-1"）
    QSet<QString> fullscreenOutputs();

    // 是否把“最大化”也算作被盖住（默认 false，只认全屏）
    void setCountMaximized(bool b) { countMaximized_ = b; }

    // 有窗口状态变化时回调（事件驱动，令遮挡判定即时生效，不必等轮询）
    void setOnChange(std::function<void()> cb) { onChange_ = std::move(cb); }

    // 调试：dump 所有 toplevel 的 app/title/状态/输出
    QString debugDump();

    // ---- Wayland listener 跳板（libwayland C 回调）----
    static void onRegistryGlobal(void *data, wl_registry *reg, uint32_t name, const char *iface, uint32_t version);
    static void onRegistryGlobalRemove(void *data, wl_registry *reg, uint32_t name);

    static void onManagerToplevel(void *data, zwlr_foreign_toplevel_manager_v1 *m, zwlr_foreign_toplevel_handle_v1 *h);
    static void onManagerFinished(void *data, zwlr_foreign_toplevel_manager_v1 *m);

    static void onToplevelTitle(void *data, zwlr_foreign_toplevel_handle_v1 *h, const char *t);
    static void onToplevelAppId(void *data, zwlr_foreign_toplevel_handle_v1 *h, const char *a);
    static void onToplevelOutputEnter(void *data, zwlr_foreign_toplevel_handle_v1 *h, wl_output *o);
    static void onToplevelOutputLeave(void *data, zwlr_foreign_toplevel_handle_v1 *h, wl_output *o);
    static void onToplevelState(void *data, zwlr_foreign_toplevel_handle_v1 *h, wl_array *state);
    static void onToplevelDone(void *data, zwlr_foreign_toplevel_handle_v1 *h);
    static void onToplevelClosed(void *data, zwlr_foreign_toplevel_handle_v1 *h);
    static void onToplevelParent(void *data, zwlr_foreign_toplevel_handle_v1 *h, zwlr_foreign_toplevel_handle_v1 *p);

    static void onOutputGeometry(void *data, wl_output *o, int32_t x, int32_t y, int32_t pw, int32_t ph, int32_t subpixel, const char *make, const char *model, int32_t transform);
    static void onOutputMode(void *data, wl_output *o, uint32_t flags, int32_t w, int32_t h, int32_t refresh);
    static void onOutputDone(void *data, wl_output *o);
    static void onOutputScale(void *data, wl_output *o, int32_t factor);
    static void onOutputName(void *data, wl_output *o, const char *name);
    static void onOutputDescription(void *data, wl_output *o, const char *description);

private:
    void notifyChange();

    struct Toplevel {
        bool fullscreen = false;
        QString app;
        QString title;
        QList<uint32_t> rawStates;
        QSet<wl_output *> outputs;
    };

    wl_display *display_ = nullptr;
    wl_registry *registry_ = nullptr;
    zwlr_foreign_toplevel_manager_v1 *manager_ = nullptr;
    bool managerFinished_ = false;
    bool countMaximized_ = false;
    std::function<void()> onChange_;
    QHash<wl_output *, QString> outputNames_;
    QHash<zwlr_foreign_toplevel_handle_v1 *, Toplevel> toplevels_;
};

} // namespace ddplugin_meme

#endif // MEME_FULLSCREENWATCH_H
