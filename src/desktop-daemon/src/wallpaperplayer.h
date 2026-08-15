// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef WALLPAPERPLAYER_H
#define WALLPAPERPLAYER_H

#include <QObject>
#include <QString>
#include <QVideoFrame>

class QWidget;
class QMediaPlayer;
class QVideoSink;

class VideoDisplayWidget;

class WallpaperPlayer : public QObject
{
    Q_OBJECT
public:
    explicit WallpaperPlayer(QObject *parent = nullptr);
    ~WallpaperPlayer();

    void setVideo(const QString &path);
    void stop();

private:
    VideoDisplayWidget *m_widget = nullptr;
    QMediaPlayer *m_player = nullptr;
    QVideoSink *m_sink = nullptr;
    QString m_savedWallpaper;
    bool m_wallpaperReplaced = false;

    void ensureWidget();
    void applyGeometry();
    void setStaticWallpaperTransparent();
    void restoreStaticWallpaper();
};

#endif // WALLPAPERPLAYER_H
