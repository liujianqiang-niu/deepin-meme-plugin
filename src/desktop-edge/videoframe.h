// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef VIDEOFRAME_H
#define VIDEOFRAME_H

#include <QImage>
#include <QByteArray>

namespace ddplugin_meme {

/** 解码输出：优先 NV12（硬解回传，无 sws），否则 RGB QImage */
struct VideoFrame {
    enum class Format {
        None = 0,
        Rgb32,   // QImage RGB32/BGRA 布局
        Nv12     // Y plane + interleaved UV
    };

    Format format = Format::None;
    int width = 0;
    int height = 0;
    QImage rgb;          // Format::Rgb32
    QByteArray yPlane;   // Format::Nv12，strideY * height
    QByteArray uvPlane;  // Format::Nv12，strideUV * ((height+1)/2)
    int strideY = 0;
    int strideUV = 0;

    bool isNull() const
    {
        if (format == Format::Rgb32)
            return rgb.isNull();
        if (format == Format::Nv12)
            return yPlane.isEmpty() || width <= 0 || height <= 0;
        return true;
    }
};

}

#endif
