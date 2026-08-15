// SPDX-License-Identifier: GPL-3.0-or-later
#include "model.h"

#include <QDir>
#include <QStandardPaths>
#include <QDebug>

static const char *kPresetDir = "/usr/share/deepin-meme-wallpapers";

WallpaperModel::WallpaperModel(QObject *parent)
    : QAbstractListModel(parent)
{
    refresh();
}

int WallpaperModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_wallpapers.size();
}

QVariant WallpaperModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_wallpapers.size())
        return {};
    const auto &entry = m_wallpapers.at(index.row());
    switch (role) {
    case NameRole:     return entry.name;
    case PathRole:     return entry.path;
    case ThumbRole:    return entry.thumb;
    case IsPresetRole: return entry.isPreset;
    }
    return {};
}

QHash<int, QByteArray> WallpaperModel::roleNames() const
{
    return {
        {NameRole,     "name"},
        {PathRole,     "path"},
        {ThumbRole,    "thumb"},
        {IsPresetRole, "isPreset"}
    };
}

int WallpaperModel::count() const
{
    return m_wallpapers.size();
}

void WallpaperModel::refresh()
{
    beginResetModel();
    m_wallpapers.clear();
    scanPreset();
    scanUser();
    endResetModel();
    emit countChanged();
}

void WallpaperModel::scanPreset()
{
    QDir dir(QString::fromUtf8(kPresetDir));
    if (!dir.exists())
        return;
    const auto files = dir.entryList({"*.mp4"}, QDir::Files, QDir::Name);
    for (const QString &file : files) {
        WallpaperEntry entry;
        entry.name = file;
        entry.path = dir.absoluteFilePath(file);
        entry.isPreset = true;
        m_wallpapers.append(entry);
    }
}

void WallpaperModel::scanUser()
{
    const QString userDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + QStringLiteral("/deepin-meme-wallpapers");
    QDir dir(userDir);
    if (!dir.exists())
        return;
    const auto files = dir.entryList({"*.mp4"}, QDir::Files, QDir::Name);
    for (const QString &file : files) {
        WallpaperEntry entry;
        entry.name = file;
        entry.path = dir.absoluteFilePath(file);
        entry.isPreset = false;
        m_wallpapers.append(entry);
    }
}

void WallpaperModel::removeUserWallpaper(int index)
{
    if (index < 0 || index >= m_wallpapers.size())
        return;
    const auto &entry = m_wallpapers.at(index);
    if (entry.isPreset) {
        qWarning() << "[meme-model] cannot remove preset wallpaper:" << entry.path;
        return;
    }
    QFile::remove(entry.path);
    beginRemoveRows({}, index, index);
    m_wallpapers.removeAt(index);
    endRemoveRows();
    emit countChanged();
}

QString WallpaperModel::pathAt(int index) const
{
    if (index < 0 || index >= m_wallpapers.size())
        return {};
    return m_wallpapers.at(index).path;
}
