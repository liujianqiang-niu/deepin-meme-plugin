// SPDX-License-Identifier: GPL-3.0-or-later
#include "decoder.h"
#include "videoframe.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QThread>
#include <cstring>
#include <algorithm>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
#include <libswscale/swscale.h>
}

using namespace ddplugin_meme;

static enum AVPixelFormat s_hwPixFmt = AV_PIX_FMT_NONE;

static enum AVPixelFormat getHwFormat(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    Q_UNUSED(ctx)
    for (const enum AVPixelFormat *p = pix_fmts; *p != -1; ++p) {
        if (*p == s_hwPixFmt)
            return *p;
    }
    return pix_fmts[0];
}

static AVBufferRef *createHwDevice(DecodeMode mode, AVHWDeviceType *outType)
{
    QList<AVHWDeviceType> candidates;
    switch (mode) {
    case DecodeMode::Cuda:
        candidates << AV_HWDEVICE_TYPE_CUDA;
        break;
    case DecodeMode::Vaapi:
        candidates << AV_HWDEVICE_TYPE_VAAPI;
        break;
    case DecodeMode::Software:
        *outType = AV_HWDEVICE_TYPE_NONE;
        return nullptr;
    case DecodeMode::Auto:
    default:
        candidates << AV_HWDEVICE_TYPE_CUDA << AV_HWDEVICE_TYPE_VAAPI;
        break;
    }

    for (AVHWDeviceType t : candidates) {
        AVBufferRef *dev = nullptr;
        if (av_hwdevice_ctx_create(&dev, t, nullptr, nullptr, 0) == 0) {
            *outType = t;
            return dev;
        }
    }
    *outType = AV_HWDEVICE_TYPE_NONE;
    return nullptr;
}

VideoDecoder::VideoDecoder(QObject *parent)
    : QThread(parent)
{
}

VideoDecoder::~VideoDecoder()
{
    requestStop();
    wait(3000);
}

void VideoDecoder::setPlaylist(const QList<QUrl> &list)
{
    QMutexLocker locker(&mutex);
    playlist = list;
}

void VideoDecoder::setOptions(const DecodeOptions &opt)
{
    QMutexLocker locker(&mutex);
    options = opt;
}

void VideoDecoder::requestStop()
{
    stopFlag.store(true);
}

void VideoDecoder::releaseFrameSlot()
{
    int v = inFlight.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (v < 0)
        inFlight.store(0, std::memory_order_release);
}

bool VideoDecoder::playOne(const QString &path)
{
    qWarning() << "[meme-wallpaper] playOne start:" << path;
    DecodeOptions opt;
    {
        QMutexLocker locker(&mutex);
        opt = options;
    }
    qWarning() << "[meme-wallpaper] playOne mode=" << int(opt.mode) << "maxWidth=" << opt.maxWidth;

    AVFormatContext *fmt = nullptr;
    if (avformat_open_input(&fmt, path.toUtf8().constData(), nullptr, nullptr) < 0) {
        qWarning() << "[meme-wallpaper] open failed:" << path;
        return false;
    }
    qWarning() << "[meme-wallpaper] playOne: avformat_open_input OK";
    if (avformat_find_stream_info(fmt, nullptr) < 0) {
        avformat_close_input(&fmt);
        return false;
    }
    qWarning() << "[meme-wallpaper] playOne: find_stream_info OK";

    int vIndex = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (vIndex < 0) {
        avformat_close_input(&fmt);
        return false;
    }
    qWarning() << "[meme-wallpaper] playOne: vIndex=" << vIndex;

    AVStream *st = fmt->streams[vIndex];
    const AVCodec *codec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!codec) {
        avformat_close_input(&fmt);
        return false;
    }

    AVCodecContext *ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        avformat_close_input(&fmt);
        return false;
    }
    qWarning() << "[meme-wallpaper] playOne: codec found" << codec->name;
    avcodec_parameters_to_context(ctx, st->codecpar);
    qWarning() << "[meme-wallpaper] playOne: params_to_context OK";

    AVHWDeviceType hwType = AV_HWDEVICE_TYPE_NONE;
    AVBufferRef *hwDev = createHwDevice(opt.mode, &hwType);
    qWarning() << "[meme-wallpaper] playOne: hwDev=" << hwDev << "hwType=" << hwType;
    bool useHw = false;
    if (hwDev) {
        for (int i = 0;; ++i) {
            const AVCodecHWConfig *cfg = avcodec_get_hw_config(codec, i);
            if (!cfg)
                break;
            if ((cfg->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
                && cfg->device_type == hwType) {
                s_hwPixFmt = cfg->pix_fmt;
                ctx->hw_device_ctx = av_buffer_ref(hwDev);
                ctx->get_format = getHwFormat;
                useHw = true;
                break;
            }
        }
        if (!useHw) {
            av_buffer_unref(&hwDev);
            hwDev = nullptr;
        }
    }

    // 硬解单线程即可；软解开一点 slice 线程
    if (useHw) {
        ctx->thread_count = 1;
        ctx->thread_type = FF_THREAD_SLICE;
    } else {
        ctx->thread_count = qMin(4, QThread::idealThreadCount());
        ctx->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
    }

    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        qWarning() << "[meme-wallpaper] playOne: avcodec_open2 failed (useHw=" << useHw << ")";
        if (useHw) {
            av_buffer_unref(&ctx->hw_device_ctx);
            ctx->hw_device_ctx = nullptr;
            ctx->get_format = nullptr;
            useHw = false;
            av_buffer_unref(&hwDev);
            hwDev = nullptr;
            ctx->thread_count = qMin(4, QThread::idealThreadCount());
            if (avcodec_open2(ctx, codec, nullptr) < 0) {
                avcodec_free_context(&ctx);
                avformat_close_input(&fmt);
                return false;
            }
        } else {
            avcodec_free_context(&ctx);
            avformat_close_input(&fmt);
            return false;
        }
    }
    qWarning() << "[meme-wallpaper] playOne: avcodec_open2 OK useHw=" << useHw;

    int srcW = ctx->width > 0 ? ctx->width : st->codecpar->width;
    int srcH = ctx->height > 0 ? ctx->height : st->codecpar->height;
    int dstW = srcW;
    int dstH = srcH;
    // maxWidth>0：超过才缩小；<=0 由上层已填成屏宽或保持原样
    if (opt.maxWidth > 0 && dstW > opt.maxWidth) {
        dstH = qMax(1, srcH * opt.maxWidth / srcW);
        dstW = opt.maxWidth;
        // 偶数对齐，部分 sws/硬解更稳
        dstW &= ~1;
        dstH &= ~1;
    }

    double srcFps = av_q2d(st->avg_frame_rate);
    if (srcFps < 1.0 || srcFps > 240.0)
        srcFps = av_q2d(st->r_frame_rate);
    if (srcFps < 1.0 || srcFps > 240.0)
        srcFps = 30.0;
    const double targetFps = (opt.fps <= 0.0) ? srcFps : qBound(opt.fps, 1.0, 240.0);
    const double speed = qBound(opt.speed, 0.01, 4.0);
    // 用微秒节拍：旧代码 qRound(1000/60)=17ms → 上限约 58.8fps，永远到不了 60
    const qint64 frameIntervalUs = qMax<qint64>(1, qRound(1000000.0 / (targetFps * speed)));
    const qint64 dropSlackUs = frameIntervalUs + frameIntervalUs / 2;

    qInfo() << "[meme-wallpaper] decode" << path
            << "hw=" << useHw
            << "type=" << (useHw ? av_hwdevice_get_type_name(hwType) : "software")
            << "src=" << srcW << "x" << srcH
            << "dst=" << dstW << "x" << dstH
            << "srcFps=" << srcFps
            << "targetFps=" << targetFps
            << "cfgFps=" << opt.fps
            << "intervalUs=" << frameIntervalUs
            << "speed=" << speed
            << (opt.fps <= 0.0 ? " FOLLOW_SOURCE" : " CAPPED");

    AVFrame *frame = av_frame_alloc();
    AVFrame *swFrame = av_frame_alloc();
    AVPacket *pkt = av_packet_alloc();

    SwsContext *sws = nullptr;
    AVPixelFormat swsFmt = AV_PIX_FMT_NONE;
    int swsW = 0, swsH = 0;

    // 壁纸缩放：默认走最快；只有明确要平滑才用重算法
    auto swsFlags = [&](int w, int h) -> int {
        if (w == dstW && h == dstH)
            return SWS_POINT;
        switch (opt.smooth) {
        case SmoothLevel::Normal: return SWS_BILINEAR;
        case SmoothLevel::High: return SWS_BICUBIC;
        case SmoothLevel::Highest: return SWS_LANCZOS;
        case SmoothLevel::Fast:
        default: return SWS_FAST_BILINEAR;
        }
    };

    // sws → BGRA/RGB32（嵌入桌面）
    AVPixelFormat swsDstFmt = AV_PIX_FMT_NV12;
    auto ensureSws = [&](enum AVPixelFormat srcFmt, int w, int h, AVPixelFormat dstFmt) -> bool {
        if (sws && swsFmt == srcFmt && swsW == w && swsH == h && swsDstFmt == dstFmt)
            return true;
        if (sws) {
            sws_freeContext(sws);
            sws = nullptr;
        }
        sws = sws_getContext(w, h, srcFmt, dstW, dstH, dstFmt,
                             swsFlags(w, h), nullptr, nullptr, nullptr);
        if (!sws)
            return false;
        swsFmt = srcFmt;
        swsW = w;
        swsH = h;
        swsDstFmt = dstFmt;
        return true;
    };

    QImage buf[2];
    int bufIdx = 0;
    inFlight.store(0);

    QElapsedTimer timer;
    timer.start();
    qint64 nextPtsUs = 0;
    int dropped = 0;
    int emitted = 0;

    auto nowUs = [&]() -> qint64 {
        return timer.nsecsElapsed() / 1000;
    };

    qWarning() << "[meme-wallpaper] playOne: entering decode loop, stopFlag=" << stopFlag.load();

    int decodeIter = 0;
    while (!stopFlag.load()) {
        int ret = av_read_frame(fmt, pkt);
        if (ret < 0) {
            av_seek_frame(fmt, vIndex, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(ctx);
            timer.restart();
            nextPtsUs = 0;
            continue;
        }
        if (pkt->stream_index != vIndex) {
            av_packet_unref(pkt);
            continue;
        }
        if (avcodec_send_packet(ctx, pkt) < 0) {
            av_packet_unref(pkt);
            continue;
        }
        av_packet_unref(pkt);

        while (!stopFlag.load() && avcodec_receive_frame(ctx, frame) == 0) {
            if ((decodeIter++ % 60) == 0)
                qWarning() << "[meme-wallpaper] playOne: decode frame" << decodeIter << "inFlight=" << inFlight.load();
            const qint64 now = nowUs();
            if (now > nextPtsUs + dropSlackUs) {
                // 落后：丢帧；时钟对齐到下一拍，避免 nextPts=now 后连续再丢半秒
                ++dropped;
                nextPtsUs += frameIntervalUs;
                if (now > nextPtsUs + dropSlackUs)
                    nextPtsUs = now;
                continue;
            }
            if (inFlight.load(std::memory_order_acquire) >= kMaxInFlight) {
                ++dropped;
                nextPtsUs += frameIntervalUs;
                const qint64 delayUs = nextPtsUs - nowUs();
                if (delayUs > 500)
                    QThread::usleep(static_cast<unsigned long>(qMin<qint64>(delayUs, 5000000)));
                else if (delayUs < -frameIntervalUs * 2)
                    nextPtsUs = nowUs();
                continue;
            }

            AVFrame *src = frame;
            if (useHw && frame->format == s_hwPixFmt) {
                if (av_hwframe_transfer_data(swFrame, frame, 0) < 0)
                    continue;
                src = swFrame;
            }

            const int w = src->width > 0 ? src->width : srcW;
            const int h = src->height > 0 ? src->height : srcH;
            // 嵌入桌面架构：解码线程完成 transfer+sws→RGB32，主线程只贴图
            const AVPixelFormat pf = static_cast<AVPixelFormat>(src->format);
            if (!ensureSws(pf, w, h, AV_PIX_FMT_BGRA))
                continue;
            QImage img;
            if (!buf[bufIdx].isNull()
                && buf[bufIdx].width() == dstW && buf[bufIdx].height() == dstH
                && buf[bufIdx].format() == QImage::Format_RGB32
                && buf[bufIdx].isDetached()) {
                img = buf[bufIdx];
            } else {
                img = QImage(dstW, dstH, QImage::Format_RGB32);
                buf[bufIdx] = img;
            }
            bufIdx ^= 1;
            if (img.isNull())
                continue;
            uint8_t *dstSlice[4] = { img.bits(), nullptr, nullptr, nullptr };
            int dstStride[4] = { int(img.bytesPerLine()), 0, 0, 0 };
            sws_scale(sws, src->data, src->linesize, 0, h, dstSlice, dstStride);

            VideoFrame out;
            out.format = VideoFrame::Format::Rgb32;
            out.width = dstW;
            out.height = dstH;
            out.rgb = img;

            if ((emitted % 180) == 0) {
                qInfo() << "[meme-wallpaper] presentFmt RGB32-desktop"
                        << out.width << "x" << out.height
                        << "inFlight" << inFlight.load();
            }
            inFlight.fetch_add(1, std::memory_order_release);
            emit frameReady(out);
            ++emitted;

            nextPtsUs += frameIntervalUs;
            const qint64 delayUs = nextPtsUs - nowUs();
            if (delayUs > 500) {
                QThread::usleep(static_cast<unsigned long>(qMin<qint64>(delayUs, 5000000)));
            } else if (delayUs < -frameIntervalUs * 2) {
                nextPtsUs = nowUs();
            }
        }
    }

    if (dropped > 0 || emitted > 0)
        qInfo() << "[meme-wallpaper] session end emitted=" << emitted << "dropped=" << dropped;

    if (sws)
        sws_freeContext(sws);
    av_packet_free(&pkt);
    av_frame_free(&swFrame);
    av_frame_free(&frame);
    avcodec_free_context(&ctx);
    av_buffer_unref(&hwDev);
    avformat_close_input(&fmt);
    return true;
}

void VideoDecoder::run()
{
    stopFlag.store(false);
    inFlight.store(0);
    while (!stopFlag.load()) {
        QList<QUrl> list;
        {
            QMutexLocker locker(&mutex);
            list = playlist;
        }
        if (list.isEmpty()) {
            QThread::msleep(500);
            continue;
        }
        for (const QUrl &url : list) {
            if (stopFlag.load())
                break;
            if (!url.isLocalFile())
                continue;
            playOne(url.toLocalFile());
        }
    }
}
