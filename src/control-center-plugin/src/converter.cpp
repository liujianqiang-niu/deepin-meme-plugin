// SPDX-License-Identifier: GPL-3.0-or-later
#include "converter.h"

#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>

VideoConverter::VideoConverter(QObject *parent)
    : QObject(parent)
{
}

bool VideoConverter::checkFormat(const QString &path)
{
    QProcess proc;
    proc.start(QStringLiteral("ffprobe"),
               { QStringLiteral("-v"), QStringLiteral("error"),
                 QStringLiteral("-select_streams"), QStringLiteral("v:0"),
                 QStringLiteral("-show_entries"), QStringLiteral("stream=codec_name"),
                 QStringLiteral("-of"), QStringLiteral("default=noprint_wrappers=1:nokey=1"),
                 path });
    if (!proc.waitForFinished(5000)) {
        qWarning() << "[meme-converter] ffprobe timeout for" << path;
        return false;
    }
    const QString codec = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    qInfo() << "[meme-converter] codec:" << codec << "path:" << path;
    return codec == QStringLiteral("h264");
}

void VideoConverter::convert(const QString &inputPath, const QString &outputDir)
{
    if (m_process) {
        qWarning() << "[meme-converter] already converting";
        return;
    }

    const QString outPath = generateOutputPath(inputPath, outputDir);
    QDir().mkpath(outputDir);

    m_totalFrames = queryTotalFrames(inputPath);
    qInfo() << "[meme-converter] convert" << inputPath << "->" << outPath
            << "totalFrames:" << m_totalFrames;

    m_process = new QProcess(this);
    const QStringList args = {
        QStringLiteral("-y"),
        QStringLiteral("-i"), inputPath,
        QStringLiteral("-c:v"), QStringLiteral("libx264"),
        QStringLiteral("-preset"), QStringLiteral("fast"),
        QStringLiteral("-crf"), QStringLiteral("23"),
        QStringLiteral("-c:a"), QStringLiteral("aac"),
        QStringLiteral("-movflags"), QStringLiteral("+faststart"),
        QStringLiteral("-progress"), QStringLiteral("pipe:1"),
        QStringLiteral("-nostats"),
        outPath
    };

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        // 解析 ffmpeg -progress 输出：frame=xxx
        const QByteArray data = m_process->readAllStandardOutput();
        for (const QByteArray &line : data.split('\n')) {
            if (line.startsWith("frame=")) {
                const int frame = line.mid(6).trimmed().toInt();
                if (m_totalFrames > 0) {
                    const int pct = qBound(0, frame * 100 / m_totalFrames, 99);
                    emit progress(pct);
                }
            }
        }
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, outPath](int code, QProcess::ExitStatus) {
        if (code == 0) {
            qInfo() << "[meme-converter] convert success:" << outPath;
            emit finished(true, outPath, {});
        } else {
            const QString err = QString::fromUtf8(m_process->readAllStandardError());
            qWarning() << "[meme-converter] convert failed:" << code << err;
            emit finished(false, {}, err);
        }
        m_process->deleteLater();
        m_process = nullptr;
        m_totalFrames = 0;
    });

    m_process->start(QStringLiteral("ffmpeg"), args);
}

void VideoConverter::cancel()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(3000);
        m_process->deleteLater();
        m_process = nullptr;
        m_totalFrames = 0;
        emit finished(false, {}, QStringLiteral("cancelled"));
    }
}

bool VideoConverter::isConverting() const
{
    return m_process != nullptr;
}

QString VideoConverter::generateOutputPath(const QString &inputPath, const QString &outputDir) const
{
    const QString baseName = QFileInfo(inputPath).completeBaseName();
    return QDir(outputDir).filePath(baseName + QStringLiteral("_h264.mp4"));
}

int VideoConverter::queryTotalFrames(const QString &path) const
{
    QProcess proc;
    proc.start(QStringLiteral("ffprobe"),
               { QStringLiteral("-v"), QStringLiteral("error"),
                 QStringLiteral("-select_streams"), QStringLiteral("v:0"),
                 QStringLiteral("-show_entries"), QStringLiteral("stream=nb_frames"),
                 QStringLiteral("-of"), QStringLiteral("default=noprint_wrappers=1:nokey=1"),
                 path });
    if (!proc.waitForFinished(5000))
        return 0;
    const QString result = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    return result.toInt();
}
