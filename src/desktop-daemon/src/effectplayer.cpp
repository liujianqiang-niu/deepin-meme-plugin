// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "effectplayer.h"
#include "themeresolver.h"
#include "thememanifest.h"

#include <QApplication>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QWidget>
#include <QScreen>
#include <QTimer>
#include <QFile>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(memePlayer, "meme.player")

EffectPlayer::EffectPlayer(meme::ThemeResolver *resolver, QObject *parent)
    : QObject(parent), m_resolver(resolver)
{
    m_overlayWindow = new QWidget();
    m_overlayWindow->setWindowFlags(
        Qt::FramelessWindowHint |
        Qt::Tool |
        Qt::WindowStaysOnTopHint |
        Qt::WindowTransparentForInput
    );
    m_overlayWindow->setAttribute(Qt::WA_TranslucentBackground);
    m_overlayWindow->setAttribute(Qt::WA_ShowWithoutActivating);
    m_overlayWindow->hide();

    m_videoWidget = new QVideoWidget(m_overlayWindow);
    m_audioOutput = new QAudioOutput(this);
    m_videoPlayer = new QMediaPlayer(this);
    m_videoPlayer->setAudioOutput(m_audioOutput);
    m_videoPlayer->setVideoOutput(m_videoWidget);

    connect(m_videoPlayer, &QMediaPlayer::playbackStateChanged, this,
        [this](QMediaPlayer::PlaybackState state) {
            if (state == QMediaPlayer::StoppedState) {
                m_overlayWindow->hide();
                m_playing = false;
            }
        });
}

EffectPlayer::~EffectPlayer()
{
    delete m_overlayWindow;
}

void EffectPlayer::setTheme(const QString &themeId)
{
    m_currentTheme = themeId;
}

void EffectPlayer::setVolume(int volume)
{
    m_volume = volume;
    if (m_audioOutput) {
        m_audioOutput->setVolume(static_cast<float>(volume) / 100.0f);
    }
}

void EffectPlayer::play(const QString &effectType, const QPoint &position,
                        QScreen *screen, const QString &filePath)
{
    Q_UNUSED(filePath)

    if (m_currentTheme.isEmpty()) {
        qCWarning(memePlayer) << "No theme selected";
        return;
    }

    // 去抖: 同类型特效 500ms 内只触发一次
    auto it = m_lastTriggerTime.find(effectType);
    if (it != m_lastTriggerTime.end() && it.value().elapsed() < DEBOUNCE_MS) {
        qCDebug(memePlayer) << "Debounced:" << effectType;
        return;
    }
    m_lastTriggerTime[effectType].start();

    // 并发限制: 上一个特效还在播则跳过
    if (m_playing) {
        qCDebug(memePlayer) << "Effect in progress, skipping";
        return;
    }

    auto typeOpt = meme::stringToEffectType(effectType);
    if (!typeOpt) {
        qCWarning(memePlayer) << "Unknown effect type:" << effectType;
        return;
    }

    auto cfg = m_resolver->effectConfig(m_currentTheme, *typeOpt);
    if (!cfg) {
        qCWarning(memePlayer) << "No effect config for theme" << m_currentTheme << "type" << effectType;
        return;
    }

    const QString themeDir = m_resolver->themeDirectory(m_currentTheme);
    const QString videoPath = meme::resolvePath(themeDir, cfg->videoPath);

    if (videoPath.isEmpty() || !QFile::exists(videoPath)) {
        qCWarning(memePlayer) << "Video not found:" << videoPath;
        return;
    }

    // 配置窗口位置与大小(考虑屏幕偏移,多显示器)
    const int size = static_cast<int>(300 * cfg->scale);
    const QPoint screenOrigin = screen ? screen->geometry().topLeft() : QPoint(0, 0);
    const QPoint globalPos = screenOrigin + position;
    const QRect geometry(globalPos.x() - size / 2, globalPos.y() - size / 2, size, size);
    m_overlayWindow->setGeometry(geometry);
    m_videoWidget->setGeometry(0, 0, size, size);

    m_videoPlayer->setSource(QUrl::fromLocalFile(videoPath));
    m_audioOutput->setVolume(static_cast<float>(m_volume) / 100.0f);

    m_playing = true;
    m_overlayWindow->show();
    m_videoPlayer->play();

    QTimer::singleShot(cfg->duration, this, [this]() {
        m_videoPlayer->stop();
        m_overlayWindow->hide();
        m_playing = false;
    });

    qCInfo(memePlayer) << "Playing:" << effectType << "video:" << videoPath
                       << "at:" << globalPos << "size:" << size;
}
