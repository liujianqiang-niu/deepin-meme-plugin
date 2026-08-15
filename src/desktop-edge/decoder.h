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

// 解码模式：Auto 优先独显 CUDA → 核显 VAAPI → 软解；config.cpp 从 DConfig 字符串转枚举
enum class DecodeMode {
    Auto = 0,      // 优先独显 CUDA，再核显 VAAPI，最后软解
    Cuda,          // NVIDIA 硬解
    Vaapi,         // 核显/通用硬解
    Software       // 软解
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
    std::atomic_int inFlight { 0 };
};

}

#endif
