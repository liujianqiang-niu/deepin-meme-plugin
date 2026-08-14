// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef WALLPAPERPLAYER_H
#define WALLPAPERPLAYER_H

#include <QObject>
#include <QString>

class QQuickView;

class WallpaperPlayer : public QObject
{
    Q_OBJECT
public:
    explicit WallpaperPlayer(QObject *parent = nullptr);
    ~WallpaperPlayer();

    void setVideo(const QString &path);
    void stop();

private:
    QQuickView *m_view = nullptr;

    void ensureView();
    void applyGeometry();
};

#endif // WALLPAPERPLAYER_H
