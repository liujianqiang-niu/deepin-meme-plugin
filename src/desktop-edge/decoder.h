// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef DECODER_H
#define DECODER_H

#include "videoframe.h"

#include <QThread>
#include <QUrl>
#include <QMutex>
#include <QList>
#include <atomic>

namespace ddplugin_meme {

// 解码模式；config.cpp 从 DConfig 字符串转枚举。
// Auto 按 VAAPI → CUDA → Vulkan → DRM 依次尝试，命中即用，全失败再回退软解。
enum class DecodeMode {
    Auto = 0,      // 自动：依次尝试下面的硬解，最后回退软解
    Vaapi,         // VAAPI：Intel / AMD / 通用（多数国产 GPU 也走这条）
    Cuda,          // CUDA：NVIDIA NVENC/NVDEC
    Vulkan,        // Vulkan Video：跨厂商 / 国产 GPU（摩尔线程、景嘉微等）
    Software       // 软解：最兼容，但最费 CPU
};

// 缩放/抗锯齿平滑等级（解码 swscale + 绘制）
enum class SmoothLevel {
    Fast = 0,      // 最快，锯齿多
    Normal,        // 均衡
    High,          // 更平滑
    Highest        // 最细（更吃 CPU）
};

struct DecodeOptions {
    int maxWidth = -1;
    double fps = 0.0;
    double speed = 1.0;
    DecodeMode mode = DecodeMode::Auto;
    SmoothLevel smooth = SmoothLevel::High;
    bool preferNv12 = false;
};

class VideoDecoder : public QThread
{
    Q_OBJECT
public:
    static constexpr int kMaxInFlight = 2;

    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder() override;

    void setPlaylist(const QList<QUrl> &list);
    void setOptions(const DecodeOptions &opt);
    void requestStop();
    /** 冻结/解冻：冻结时不读包、不出帧，但保留文件与硬解上下文，解冻后从当前帧继续 */
    void setPaused(bool p) { pausedFlag.store(p, std::memory_order_release); }
    void releaseFrameSlot();

signals:
    void frameReady(const VideoFrame &frame);

protected:
    void run() override;

private:
    bool playOne(const QString &path);

    QList<QUrl> playlist;
    DecodeOptions options;
    QMutex mutex;
    std::atomic_bool stopFlag { false };
    std::atomic_bool pausedFlag { false };
    std::atomic_int inFlight { 0 };
};

}

#endif
