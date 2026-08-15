// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEME_WALLPAPER_MODEL_H
#define MEME_WALLPAPER_MODEL_H

#include <QAbstractListModel>
#include <QStringList>

struct WallpaperEntry {
    QString name;    // 文件名
    QString path;    // 绝对路径
    QString thumb;   // 缩略图路径（ffprobe 首帧，暂留空）
    bool isPreset = true;   // true=预置, false=用户上传
};

class WallpaperModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        ThumbRole,
        IsPresetRole
    };

    explicit WallpaperModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void removeUserWallpaper(int index);
    Q_INVOKABLE QString pathAt(int index) const;

signals:
    void countChanged();

private:
    QList<WallpaperEntry> m_wallpapers;
    void scanPreset();
    void scanUser();
};

#endif // MEME_WALLPAPER_MODEL_H
