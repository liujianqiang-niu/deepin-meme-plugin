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
    void convertingChanged();
    void convertProgressChanged();
    void statusMessageChanged();

private:
    WallpaperModel *m_model = nullptr;
    VideoConverter *m_converter = nullptr;

    bool m_enabled = false;
    QString m_currentVideo;
    int m_convertProgress = 0;
    QString m_statusMessage;

    void setStatusMessage(const QString &msg);
    void readConfig();
    void writeConfigEnabled(bool e);
    void writeConfigCurrentVideo(const QString &path);
};

#endif // MEMEPLUGIN_H
