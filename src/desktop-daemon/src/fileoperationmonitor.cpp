// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fileoperationmonitor.h"

#include <QFileSystemWatcher>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QFileDevice>

#include <sys/stat.h>

Q_LOGGING_CATEGORY(memeMonitor, "meme.monitor")

namespace {
class WatcherHolder : public QObject {
public:
    QFileSystemWatcher *watcher;
    explicit WatcherHolder(QObject *parent) : QObject(parent) {
        watcher = new QFileSystemWatcher(this);
    }
};

qint64 getInode(const QFileInfo &info)
{
    // Qt6.8 QFileInfo 没有 fileId(), 用 stat() 获取 inode
    struct stat st;
    if (stat(info.absoluteFilePath().toLocal8Bit().constData(), &st) == 0) {
        return static_cast<qint64>(st.st_ino);
    }
    return 0;
}
} // namespace

FileOperationMonitor::FileOperationMonitor(QObject *parent)
    : QObject(parent)
{
}

QString FileOperationMonitor::desktopPath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (dir.isEmpty()) dir = QDir::homePath() + QStringLiteral("/Desktop");
    return dir;
}

void FileOperationMonitor::start()
{
    if (m_running) return;

    auto *holder = new WatcherHolder(this);
    auto *watcher = holder->watcher;

    const QString desktopDir = desktopPath();
    qCInfo(memeMonitor) << "Monitoring desktop:" << desktopDir;

    watcher->addPath(desktopDir);
    scanDesktop();

    connect(watcher, &QFileSystemWatcher::directoryChanged, this,
        [this](const QString &path) { onDirectoryChanged(path); });

    m_running = true;
}

void FileOperationMonitor::stop()
{
    m_running = false;
}

void FileOperationMonitor::scanDesktop()
{
    m_snapshot.clear();
    const QDir dir(desktopPath());
    const auto entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &info : entries) {
        FileSnapshot snap;
        snap.path = info.absoluteFilePath();
        snap.inode = getInode(info);
        snap.size = info.size();
        m_snapshot.insert(info.absoluteFilePath(), snap);
    }
}

void FileOperationMonitor::onDirectoryChanged(const QString &path)
{
    Q_UNUSED(path)

    const QDir dir(desktopPath());
    const auto newEntries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    QHash<QString, FileSnapshot> newSnapshot;
    QHash<qint64, QString> inodeToNewPath;
    for (const QFileInfo &info : newEntries) {
        FileSnapshot snap;
        snap.path = info.absoluteFilePath();
        snap.inode = getInode(info);
        snap.size = info.size();
        newSnapshot.insert(info.absoluteFilePath(), snap);
        if (snap.inode > 0) {
            inodeToNewPath.insert(snap.inode, info.absoluteFilePath());
        }
    }

    // 检测删除/重命名
    QStringList deletedPaths;
    QHash<qint64, QString> deletedInodes;
    for (auto it = m_snapshot.begin(); it != m_snapshot.end(); ++it) {
        if (!newSnapshot.contains(it.key())) {
            deletedPaths.append(it.key());
            if (it.value().inode > 0) {
                deletedInodes.insert(it.value().inode, it.key());
            }
        }
    }

    // 检测新建
    QStringList createdPaths;
    for (auto it = newSnapshot.begin(); it != newSnapshot.end(); ++it) {
        if (!m_snapshot.contains(it.key())) {
            createdPaths.append(it.key());
        }
    }

    // 重命名检测: 旧 inode 在新快照中找到了对应的新路径
    for (auto it = deletedInodes.begin(); it != deletedInodes.end(); ++it) {
        auto newIt = inodeToNewPath.find(it.key());
        if (newIt != inodeToNewPath.end() && newIt.value() != it.value()) {
            qCInfo(memeMonitor) << "Detected rename:" << it.value() << "->" << newIt.value();
            emit fileOperationDetected(QStringLiteral("rename"), newIt.value());
            deletedPaths.removeOne(it.value());
            createdPaths.removeOne(newIt.value());
        }
    }

    // 发射删除事件
    for (const QString &p : deletedPaths) {
        qCInfo(memeMonitor) << "Detected delete:" << p;
        emit fileOperationDetected(QStringLiteral("delete"), p);
    }

    // 发射新建事件 (move/copy 从外部进入桌面,统一识别为 create)
    for (const QString &p : createdPaths) {
        qCInfo(memeMonitor) << "Detected create:" << p;
        emit fileOperationDetected(QStringLiteral("create"), p);
    }

    m_snapshot = newSnapshot;
}
