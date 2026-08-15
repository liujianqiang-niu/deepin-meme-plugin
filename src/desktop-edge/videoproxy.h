// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEME_VIDEOPROXY_H
#define MEME_VIDEOPROXY_H

#include "global.h"
#include "decoder.h"
#include "videoframe.h"

#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QElapsedTimer>
#include <QSharedPointer>
#include <functional>

namespace ddplugin_meme {

// 画面铺屏方式（相对屏幕）；从 DConfig 字符串转枚举，由 engine 下发给 proxy
enum class FillMode {
    Fill = 0,      // 铺满：等比放大裁切，无黑边（默认）
    Fit,           // 自适应：完整显示，可能有黑边
    Stretch,       // 拉伸：拉满屏，可能变形
    Center,        // 居中：原始像素居中，不缩放
    Tile           // 平铺：重复铺满
};

struct PlayOptions {
    DecodeMode mode = DecodeMode::Software;
    SmoothLevel smooth = SmoothLevel::Fast;
    FillMode fill = FillMode::Fill;
    double speed = 1.0;
    double fps = 0.0;
    int maxWidth = -1;
};

/**
 * 嵌入桌面 root 的 QWidget（禁止 QOpenGLWidget / 独立窗）。
 * 呈现用 QPixmap + drawPixmap：X11 上比每帧 drawImage 更贴合成路径。
 */
class VideoProxy : public QWidget
{
    Q_OBJECT
public:
    explicit VideoProxy(QWidget *parent = nullptr);
    ~VideoProxy() override;

    void stop();
    void updateImage(const QImage &img);
    /** 多屏共享同一 QPixmap（主线程只 fromImage 一次） */
    void presentPixmap(const QPixmap &pm, int srcW, int srcH,
                       const std::function<void()> &painted = {});
    void present(const VideoFrame &frame, const std::function<void()> &painted = {});
    void refreshOverlay();

    // 配置由 engine 从 DConfig 读取后下发，proxy 不直接访问配置单例
    void setFillMode(FillMode mode);
    void setShowFps(bool show);
    void setFpsCap(double fps);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void drawFpsOverlay(QPainter &pa);
    void armPaint(int srcW, int srcH);

    QPixmap pixmap;
    QElapsedTimer paintGate;
    QElapsedTimer fpsClock;
    qint64 nextPaintUs = 0;
    int framesInWindow = 0;
    double displayFps = 0.0;
    int lastFrameW = 0;
    int lastFrameH = 0;
    bool paintScheduled = false;
    std::function<void()> afterPaint;

    FillMode fillMode = FillMode::Fill;
    bool showFps = false;
    double fpsCap = 0.0;   // 0 = 不限速（跟随解码线程节拍）
};

typedef QSharedPointer<VideoProxy> VideoProxyPointer;

}

#endif
