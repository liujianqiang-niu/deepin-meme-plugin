// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import org.deepin.dcc 1.0
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

// 主页面: 可以使用 dccData（C++ 后端）
DccObject {
    // 开关: 是否启用趣味壁纸特效
    DccObject {
        name: "memeEnabled"
        parentName: "meme"
        displayName: qsTr("启用特效")
        weight: 10
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: Switch {
            checked: dccData.enabled
            onCheckedChanged: {
                dccData.enabled = checked
            }
        }
    }

    // 主题包选择
    DccObject {
        name: "memeTheme"
        parentName: "meme"
        displayName: qsTr("特效主题")
        weight: 20
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: ComboBox {
            model: dccData.themeList
            currentIndex: {
                const idx = dccData.themeList.indexOf(dccData.currentTheme)
                return idx >= 0 ? idx : 0
            }
            onActivated: {
                dccData.currentTheme = currentText
            }
        }
    }

    // 实时预览窗口
    DccObject {
        name: "memePreview"
        parentName: "meme"
        displayName: qsTr("预览")
        weight: 30
        pageType: DccObject.Item
        backgroundType: DccObject.Normal
        page: MemePreview {
            themeId: dccData.currentTheme
            enabled: dccData.enabled
            // 预览演示特效
            onPreviewEffectRequested: (effectType) => {
                dccData.previewEffect(effectType)
            }
        }
    }

    // 高级设置
    DccObject {
        name: "memeSettings"
        parentName: "meme"
        displayName: qsTr("高级设置")
        weight: 40
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: Slider {
            from: 0
            to: 100
            value: dccData.effectVolume
            onMoved: {
                dccData.effectVolume = value
            }
        }
    }
}
