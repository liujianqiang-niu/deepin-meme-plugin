// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef WALLpaperPLAYER_H
#define WALLPAPERPLAYER_H

#include <QObject>
#include <QString>

class QMediaPlayer;
class QVideoWidget;
class QWidget;

class WallpaperPlayer : public QObject
{
    Q_OBJECT
public:
    explicit WallpaperPlayer(QObject *parent = nullptr);
    ~WallpaperPlayer();

    void setVideo(const QString &path);
    void stop();

private:
    QWidget *m_window = nullptr;
    QVideoWidget *m_videoWidget = nullptr;
    QMediaPlayer *m_player = nullptr;
};

#endif // WALLPAPERPLAYER_H
