// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef EFFECTPLAYER_H
#define EFFECTPLAYER_H

#include <QObject>
#include <QPoint>
#include <QHash>
#include <QElapsedTimer>

namespace meme {
class ThemeResolver;
}

class QMediaPlayer;
class QVideoWidget;
class QWidget;
class QScreen;
class QAudioOutput;
class QTimer;

class EffectPlayer : public QObject
{
    Q_OBJECT
public:
    explicit EffectPlayer(meme::ThemeResolver *resolver, QObject *parent = nullptr);
    ~EffectPlayer();

    void setTheme(const QString &themeId);
    void setVolume(int volume);
    meme::ThemeResolver *themeResolver() const { return m_resolver; }

    // 在 point 位置(指定 screen 上)播放特效
    void play(const QString &effectType, const QPoint &position,
              QScreen *screen, const QString &filePath);

private:
    meme::ThemeResolver *m_resolver;
    QString m_currentTheme;
    int m_volume = 80;

    QMediaPlayer *m_videoPlayer = nullptr;
    QVideoWidget *m_videoWidget = nullptr;
    QWidget *m_overlayWindow = nullptr;
    QAudioOutput *m_audioOutput = nullptr;

    // 性能优化: 去抖 - 同类型特效 500ms 内只触发一次
    QHash<QString, QElapsedTimer> m_lastTriggerTime;
    static constexpr int DEBOUNCE_MS = 500;

    // 并发限制: 同时只播放一个特效
    bool m_playing = false;
};

#endif // EFFECTPLAYER_H
