// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEMEDCONFIG_H
#define MEMEDCONFIG_H

#include <QObject>
#include <QVariant>

namespace meme {

// DConfig 持久化封装,基于 DTK6 DConfig
class MemeDConfig : public QObject
{
    Q_OBJECT
public:
    explicit MemeDConfig(QObject *parent = nullptr);
    ~MemeDConfig();

    bool isValid() const;

    bool enabled() const;
    void setEnabled(bool e);

    QString currentTheme() const;
    void setCurrentTheme(const QString &id);

    int effectVolume() const;
    void setEffectVolume(int vol);

signals:
    void changed(const QString &key);

private:
    // DConfig* 实际类型,定义为 void* 以避免头文件中暴露 dconfig.h
    void *m_dconfig = nullptr;
};

} // namespace meme

#endif // MEMEDCONFIG_H
