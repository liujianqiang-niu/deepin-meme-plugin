// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEME_EDGE_CONFIG_H
#define MEME_EDGE_CONFIG_H

#include "global.h"
#include "decoder.h"       // DecodeMode
#include "videoproxy.h"    // FillMode

#include <QObject>
#include <QString>

namespace ddplugin_meme {

/**
 * DConfig 读取器：跨进程共享配置（edge 插件 + 控制中心）。
 * schema 文件: data/configs/org.deepin.meme.json
 * appId = name = "org.deepin.meme"
 */
class MemeConfig : public QObject
{
    Q_OBJECT
public:
    explicit MemeConfig(QObject *parent = nullptr);
    ~MemeConfig() override;

    bool isValid() const;

    // --- 读取 ---
    bool enabled() const;
    QString currentVideo() const;
    DecodeMode decodeMode() const;
    FillMode fillMode() const;

    // --- 写入 ---
    void setEnabled(bool e);
    void setCurrentVideo(const QString &path);
    void setDecodeMode(DecodeMode mode);
    void setFillMode(FillMode mode);

    // --- 字符串 ↔ 枚举转换 ---
    static DecodeMode decodeModeFromString(const QString &s);
    static QString decodeModeToString(DecodeMode m);
    static FillMode fillModeFromString(const QString &s);
    static QString fillModeToString(FillMode m);

signals:
    void configChanged();

private:
    class DConfig *m_cfg = nullptr;
};

}

#endif // MEME_EDGE_CONFIG_H
