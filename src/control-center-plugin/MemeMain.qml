// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import org.deepin.dcc 1.0
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

DccObject {
    DccObject {
        name: "memeEnabled"
        parentName: "meme"
        displayName: qsTr("启用特效")
        weight: 10
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: Switch {
            checked: dccData.enabled
            onCheckedChanged: dccData.enabled = checked
        }
    }

    DccObject {
        name: "memeTheme"
        parentName: "meme"
        displayName: qsTr("特效主题")
        weight: 20
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: RowLayout {
            spacing: 8
            ComboBox {
                id: themeCombo
                Layout.fillWidth: true
                model: dccData.themeModel
                textRole: "name"
                valueRole: "id"
                currentIndex: {
                    for (var i = 0; i < dccData.themeModel.length; i++) {
                        if (dccData.themeModel[i].id === dccData.currentTheme)
                            return i
                    }
                    return 0
                }
                property string selectedTheme: dccData.currentTheme
                onActivated: selectedTheme = currentValue
            }
            Button {
                text: qsTr("应用")
                enabled: themeCombo.selectedTheme !== "" && themeCombo.selectedTheme !== dccData.currentTheme
                onClicked: {
                    dccData.currentTheme = themeCombo.selectedTheme
                }
            }
        }
    }

    DccObject {
        name: "memePreview"
        parentName: "meme"
        displayName: qsTr("效果预览")
        weight: 30
        pageType: DccObject.Item
        backgroundType: DccObject.Normal
        page: MemePreview {
            id: memePreview
            themeId: dccData.currentTheme
            enabled: dccData.enabled
            wallpaperUrl: dccData.enabled ? dccData.previewVideoUrl(dccData.currentTheme) : ""
            onPreviewEffectRequested: (effectType) => {
                var url = dccData.effectVideoUrl(dccData.currentTheme, effectType)
                memePreview.playEffect(url)
                dccData.previewEffect(effectType)
            }
        }
    }

    DccObject {
        name: "memeSettings"
        parentName: "meme"
        displayName: qsTr("高级设置")
        weight: 40
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: ColumnLayout {
            spacing: 6
            Label {
                text: qsTr("音效音量：调节特效播放时的音量大小")
                font.pixelSize: 12
                color: "#888"
                Layout.fillWidth: true
            }
            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("静音"); font.pixelSize: 11; color: "#aaa" }
                Slider {
                    Layout.fillWidth: true
                    from: 0; to: 100
                    value: dccData.effectVolume
                    onMoved: dccData.effectVolume = value
                }
                Label { text: qsTr("最大"); font.pixelSize: 11; color: "#aaa" }
            }
        }
    }
}
