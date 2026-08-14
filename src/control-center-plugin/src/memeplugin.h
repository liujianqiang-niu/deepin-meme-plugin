// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEMEPLUGIN_H
#define MEMEPLUGIN_H

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QUrl>

struct WallpaperEntry {
    QString name;
    QString path;
};

class MemePlugin : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString currentVideo READ currentVideo WRITE setCurrentVideo NOTIFY currentVideoChanged)
    Q_PROPERTY(QVariantList wallpaperModel READ wallpaperModel NOTIFY wallpaperModelChanged)

public:
    explicit MemePlugin(QObject *parent = nullptr);
    ~MemePlugin();

    bool enabled() const;
    void setEnabled(bool e);

    QString currentVideo() const;
    void setCurrentVideo(const QString &path);

    QVariantList wallpaperModel() const;

    Q_INVOKABLE void applyWallpaper(const QString &path);
    Q_INVOKABLE QUrl urlFromPath(const QString &path) const;

signals:
    void enabledChanged(bool);
    void currentVideoChanged(const QString &);
    void wallpaperModelChanged();

private:
    void loadWallpaperList();

    QList<WallpaperEntry> m_wallpapers;
    bool m_enabled = false;
    QString m_currentVideo;
};

#endif // MEMEPLUGIN_H
