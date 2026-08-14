// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef EFFECTOVERLAY_H
#define EFFECTOVERLAY_H

#include <QObject>
#include <QDBusContext>
#include <QHash>
#include <QPoint>

class EffectPlayer;
class WallpaperManager;
class QScreen;

// 特效叠加层: 透明窗口在桌面上播放特效动画
// 同时作为 D-Bus 对象暴露 org.deepin.meme.daemon 接口
class EffectOverlay : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.meme.daemon")

public:
    explicit EffectOverlay(EffectPlayer *player, WallpaperManager *wallpaperMgr, QObject *parent = nullptr);
    ~EffectOverlay();

    // 触发特效: effectType = "delete"/"create"/"rename"/"move"/"copy"
    void triggerEffect(const QString &effectType, const QString &filePath);

public slots:
    // D-Bus 方法
    void SetEnabled(bool enabled);
    void SetTheme(const QString &themeId);
    QString GetTheme();
    QStringList GetThemes();
    void PreviewEffect(const QString &effectType);
    void SetEffectVolume(int volume);
    void ReloadThemes();
    bool GetEnabled();

signals:
    void ThemeChanged(const QString &themeId);
    void EffectTriggered(const QString &effectType, int x, int y);

private:
    // 根据 filePath 查找文件所在的 QScreen (多显示器支持)
    QScreen *findScreenForFile(const QString &filePath) const;

    // 尝试通过 D-Bus 调用 org.deepin.dde.desktop.canvas 获取文件在桌面的精确坐标
    // 失败返回空 QPoint(-1,-1),调用方回退到屏幕中心
    QPoint tryGetFileCoordinate(const QString &filePath) const;

    EffectPlayer *m_player;
    WallpaperManager *m_wallpaperManager = nullptr;
    bool m_enabled = false;
    QString m_currentTheme;
    int m_effectVolume = 80;
};

#endif // EFFECTOVERLAY_H
