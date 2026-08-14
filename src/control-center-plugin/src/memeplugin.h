// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEMEPLUGIN_H
#define MEMEPLUGIN_H

#include <QObject>
#include <QStringList>
#include <QVariantList>

namespace meme {
class MemeThemeManager;
}

// 控制中心插件后端，通过 dccData 暴露给 QML
class MemePlugin : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString currentTheme READ currentTheme WRITE setCurrentTheme NOTIFY currentThemeChanged)
    Q_PROPERTY(QStringList themeList READ themeList NOTIFY themeListChanged)
    Q_PROPERTY(QVariantList themeModel READ themeModel NOTIFY themeListChanged)
    Q_PROPERTY(int effectVolume READ effectVolume WRITE setEffectVolume NOTIFY effectVolumeChanged)

public:
    explicit MemePlugin(QObject *parent = nullptr);
    ~MemePlugin();

    bool enabled() const;
    void setEnabled(bool e);

    QString currentTheme() const;
    void setCurrentTheme(const QString &id);

    QStringList themeList() const;
    QVariantList themeModel() const;
    int effectVolume() const;
    void setEffectVolume(int vol);

    // QML 可调用: 返回主题的预览壁纸视频 URL
    Q_INVOKABLE QString previewVideoUrl(const QString &themeId) const;

    // QML 可调用: 返回主题指定特效的视频 URL
    Q_INVOKABLE QString effectVideoUrl(const QString &themeId, const QString &effectType) const;

    // QML 可调用: 预览指定类型的特效
    Q_INVOKABLE void previewEffect(const QString &effectType);

signals:
    void enabledChanged(bool);
    void currentThemeChanged(const QString &);
    void themeListChanged();
    void effectVolumeChanged(int);

private:
    meme::MemeThemeManager *m_themeManager;
    bool m_enabled = false;
    QString m_currentTheme;
    int m_effectVolume = 80;
};

#endif // MEMEPLUGIN_H
