// SPDX-License-Identifier: GPL-3.0-or-later
#include "videoproxy.h"

#include <QPainter>
#include <QPaintEvent>
#include <QFont>

using namespace ddplugin_meme;

VideoProxy::VideoProxy(QWidget *parent)
    : QWidget(parent)
{
    // 钉死嵌入：绝不当顶层窗
    setWindowFlags(Qt::Widget);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_DontCreateNativeAncestors, true);
    setAutoFillBackground(false);
    paintGate.start();
    fpsClock.start();
}

VideoProxy::~VideoProxy()
{
    stop();
}

void VideoProxy::stop()
{
    pixmap = QPixmap();
    framesInWindow = 0;
    displayFps = 0.0;
    lastFrameW = lastFrameH = 0;
    nextPaintUs = 0;
    paintScheduled = false;
    afterPaint = {};
    update();
}

void VideoProxy::setFillMode(FillMode mode)
{
    fillMode = mode;
}

void VideoProxy::setShowFps(bool show)
{
    showFps = show;
}

void VideoProxy::setFpsCap(double fps)
{
    fpsCap = fps;
}

void VideoProxy::refreshOverlay()
{
    if (!paintScheduled) {
        paintScheduled = true;
        update();
    }
}

void VideoProxy::armPaint(int srcW, int srcH)
{
    lastFrameW = srcW;
    lastFrameH = srcH;
    ++framesInWindow;
    const qint64 elapsed = fpsClock.elapsed();
    if (elapsed >= 1000) {
        displayFps = framesInWindow * 1000.0 / double(elapsed);
        framesInWindow = 0;
        fpsClock.restart();
    }
    if (!paintScheduled) {
        paintScheduled = true;
        update();
    }
}

void VideoProxy::presentPixmap(const QPixmap &pm, int srcW, int srcH,
                               const std::function<void()> &painted)
{
    if (pm.isNull())
        return;
    pixmap = pm;
    afterPaint = painted;
    armPaint(srcW > 0 ? srcW : pm.width(), srcH > 0 ? srcH : pm.height());
}

void VideoProxy::present(const VideoFrame &vf, const std::function<void()> &painted)
{
    if (vf.isNull())
        return;
    // 架构：色彩转换只在解码线程；此处只收 RGB
    if (vf.format != VideoFrame::Format::Rgb32 || vf.rgb.isNull())
        return;
    presentPixmap(QPixmap::fromImage(vf.rgb), vf.width, vf.height, painted);
}

void VideoProxy::updateImage(const QImage &img)
{
    if (img.isNull())
        return;
    if (fpsCap > 0.0) {
        if (!paintGate.isValid())
            paintGate.start();
        const qint64 minGapUs = qMax<qint64>(1, qRound(1000000.0 / fpsCap));
        const qint64 now = paintGate.nsecsElapsed() / 1000;
        if (now < nextPaintUs)
            return;
        nextPaintUs = now + minGapUs;
    }
    presentPixmap(QPixmap::fromImage(img), img.width(), img.height(), {});
}

void VideoProxy::drawFpsOverlay(QPainter &pa)
{
    if (!showFps)
        return;
    const QString target = (fpsCap <= 0.0)
            ? QStringLiteral("设置:原始")
            : QStringLiteral("设置:%1").arg(qRound(fpsCap));
    const QString text = QStringLiteral("%1 fps | %2 | %3x%4")
            .arg(displayFps, 0, 'f', 1).arg(target).arg(lastFrameW).arg(lastFrameH);
    QFont font = pa.font();
    font.setPointSize(13);
    font.setBold(true);
    pa.setFont(font);
    const QFontMetrics fm(font);
    const int pad = 8;
    const QRect box(12, 12, fm.horizontalAdvance(text) + pad * 2, fm.height() + pad * 2);
    pa.setPen(Qt::NoPen);
    pa.setBrush(QColor(0, 0, 0, 170));
    pa.drawRoundedRect(box, 6, 6);
    pa.setPen(QColor(0, 255, 120));
    pa.drawText(box, Qt::AlignCenter, text);
}

void VideoProxy::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    paintScheduled = false;
    std::function<void()> done = std::move(afterPaint);
    afterPaint = {};

    QPainter pa(this);
    if (pixmap.isNull()) {
        pa.fillRect(rect(), Qt::black);
        drawFpsOverlay(pa);
        if (done) done();
        return;
    }

    pa.setRenderHint(QPainter::SmoothPixmapTransform, false);
    const FillMode mode = fillMode;
    const QSize ps = pixmap.size();
    const QSize ws = size();

    // 1:1：零缩放
    if (ps == ws) {
        pa.drawPixmap(0, 0, pixmap);
        drawFpsOverlay(pa);
        if (done) done();
        return;
    }

    switch (mode) {
    case FillMode::Fit: {
        const QSize tar = ps.scaled(ws, Qt::KeepAspectRatio);
        const int x = (ws.width() - tar.width()) / 2;
        const int y = (ws.height() - tar.height()) / 2;
        if (tar != ws)
            pa.fillRect(rect(), Qt::black);
        pa.drawPixmap(QRect(x, y, tar.width(), tar.height()), pixmap);
        break;
    }
    case FillMode::Stretch:
        pa.drawPixmap(rect(), pixmap);
        break;
    case FillMode::Center:
        pa.fillRect(rect(), Qt::black);
        pa.drawPixmap((ws.width() - ps.width()) / 2, (ws.height() - ps.height()) / 2, pixmap);
        break;
    case FillMode::Tile:
        pa.drawTiledPixmap(rect(), pixmap);
        break;
    case FillMode::Fill:
    default: {
        const QSize tar = ps.scaled(ws, Qt::KeepAspectRatioByExpanding);
        const int x = (ws.width() - tar.width()) / 2;
        const int y = (ws.height() - tar.height()) / 2;
        pa.drawPixmap(QRect(x, y, tar.width(), tar.height()), pixmap);
        break;
    }
    }
    drawFpsOverlay(pa);
    if (done) done();
}
