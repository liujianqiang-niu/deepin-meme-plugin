// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEMEDCONFIG_H
#define MEMEDCONFIG_H

#include <QObject>
#include <QString>

namespace meme {

class MemeDConfig : public QObject
{
    Q_OBJECT
public:
    explicit MemeDConfig(QObject *parent = nullptr);
    ~MemeDConfig();

    bool isValid() const;

    bool enabled() const;
    void setEnabled(bool e);

    QString currentVideo() const;
    void setCurrentVideo(const QString &path);

signals:
    void changed(const QString &key);

private:
    void *m_dconfig = nullptr;
};

} // namespace meme

#endif // MEMEDCONFIG_H
