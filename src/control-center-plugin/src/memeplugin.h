// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEMEPLUGIN_H
#define MEMEPLUGIN_H

#include "model.h"
#include "converter.h"

#include <QObject>
#include <QString>
#include <QUrl>

class MemePlugin : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString currentVideo READ currentVideo WRITE setCurrentVideo NOTIFY currentVideoChanged)
    Q_PROPERTY(bool pauseOnMaximized READ pauseOnMaximized WRITE setPauseOnMaximized NOTIFY pauseOnMaximizedChanged)
    Q_PROPERTY(int fpsCap READ fpsCap WRITE setFpsCap NOTIFY fpsCapChanged)
    Q_PROPERTY(int renderWidth READ renderWidth WRITE setRenderWidth NOTIFY renderWidthChanged)
    Q_PROPERTY(QString decodeMode READ decodeMode WRITE setDecodeMode NOTIFY decodeModeChanged)
    Q_PROPERTY(WallpaperModel *wallpaperModel READ wallpaperModel CONSTANT)
    Q_PROPERTY(bool converting READ converting NOTIFY convertingChanged)
    Q_PROPERTY(int convertProgress READ convertProgress NOTIFY convertProgressChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit MemePlugin(QObject *parent = nullptr);
    ~MemePlugin() override;

    bool enabled() const;
    void setEnabled(bool e);

    QString currentVideo() const;
    void setCurrentVideo(const QString &path);

    bool pauseOnMaximized() const;
    void setPauseOnMaximized(bool b);

    int fpsCap() const;
    void setFpsCap(int v);
    int renderWidth() const;
    void setRenderWidth(int v);

    QString decodeMode() const;
    void setDecodeMode(const QString &m);

    WallpaperModel *wallpaperModel() const;
    bool converting() const;
    int convertProgress() const;
    QString statusMessage() const;

    Q_INVOKABLE void applyWallpaper(const QString &path);
    Q_INVOKABLE QUrl urlFromPath(const QString &path) const;
    Q_INVOKABLE void uploadVideo(const QUrl &url);
    Q_INVOKABLE void removeUserWallpaper(int index);
    Q_INVOKABLE void cancelConvert();

signals:
    void enabledChanged(bool);
    void currentVideoChanged(const QString &);
    void pauseOnMaximizedChanged(bool);
    void fpsCapChanged(int);
    void renderWidthChanged(int);
    void decodeModeChanged(const QString &);
    void convertingChanged();
    void convertProgressChanged();
    void statusMessageChanged();

private:
    WallpaperModel *m_model = nullptr;
    VideoConverter *m_converter = nullptr;

    bool m_enabled = false;
    QString m_currentVideo;
    bool m_pauseOnMaximized = false;
    int m_fpsCap = 0;
    int m_renderWidth = 0;
    QString m_decodeMode = QStringLiteral("auto");
    int m_convertProgress = 0;
    bool m_converting = false;
    QString m_statusMessage;

    void setStatusMessage(const QString &msg);
    void readConfig();
    void writeConfigEnabled(bool e);
    void writeConfigCurrentVideo(const QString &path);
    void writeConfigPauseOnMaximized(bool b);
    void writeConfigFpsCap(int v);
    void writeConfigRenderWidth(int v);
    void writeConfigDecodeMode(const QString &m);
};

#endif // MEMEPLUGIN_H
