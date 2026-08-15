// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEME_VIDEO_CONVERTER_H
#define MEME_VIDEO_CONVERTER_H

#include <QObject>
#include <QString>

class QProcess;

class VideoConverter : public QObject
{
    Q_OBJECT
public:
    explicit VideoConverter(QObject *parent = nullptr);
    ~VideoConverter();

    /** 检查视频是否为 H264 编码，返回 true 表示符合壁纸引擎要求 */
    static bool checkFormat(const QString &path);

    /** 转换为 H264 MP4，异步发出 progress 和 finished 信号 */
    void convert(const QString &inputPath, const QString &outputDir);
    void cancel();

    bool isConverting() const;

signals:
    void progress(int percent);
    void finished(bool success, const QString &outputPath, const QString &error);

private:
    QProcess *m_process = nullptr;
    int m_totalFrames = 0;

    QString generateOutputPath(const QString &inputPath, const QString &outputDir) const;
    int queryTotalFrames(const QString &path) const;
};

#endif // MEME_VIDEO_CONVERTER_H
