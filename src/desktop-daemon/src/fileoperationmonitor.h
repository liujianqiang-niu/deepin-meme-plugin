// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef FILEOPERATIONMONITOR_H
#define FILEOPERATIONMONITOR_H

#include <QObject>
#include <QHash>
#include <QFileInfo>
#include <QStringList>

// 文件操作监听器: 监听桌面目录的文件变更
// 通过 QFileSystemWatcher + 快照对比推断操作类型:
//   - delete: 旧快照有,新快照无
//   - create: 旧快照无,新快照有
//   - rename: 同一目录下文件消失+新文件出现,且 inode 相同
//   - move/copy: 简化为 create (外部源移入/复制入桌面)
class FileOperationMonitor : public QObject
{
    Q_OBJECT
public:
    explicit FileOperationMonitor(QObject *parent = nullptr);

    void start();
    void stop();

signals:
    void fileOperationDetected(const QString &operationType, const QString &filePath);

private slots:
    void onDirectoryChanged(const QString &path);

private:
    void scanDesktop();
    QString desktopPath() const;

    // 快照: filePath -> (absoluteFilePath, inode, size, lastModified)
    struct FileSnapshot {
        QString path;
        qint64 inode = 0;
        qint64 size = 0;
    };
    QHash<QString, FileSnapshot> m_snapshot;
    bool m_running = false;
};

#endif // FILEOPERATIONMONITOR_H
