// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import org.deepin.dcc 1.0
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtMultimedia

DccObject {
    DccObject {
        name: "memeEnabled"
        parentName: "meme"
        displayName: qsTr("启用动态壁纸")
        weight: 10
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: Switch {
            checked: dccData.enabled
            onCheckedChanged: dccData.enabled = checked
        }
    }

    DccObject {
        name: "memeWallpaper"
        parentName: "meme"
        displayName: qsTr("动态壁纸")
        weight: 20
        pageType: DccObject.Item
        backgroundType: DccObject.Normal
        page: ColumnLayout {
            spacing: 8
            Layout.fillWidth: true

            Rectangle {
                id: previewContainer
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                color: "#000"
                clip: true
                radius: 6

                Video {
                    id: previewVideo
                    anchors.fill: parent
                    fillMode: VideoOutput.PreserveAspectFit
                    visible: false
                    muted: true
                    loops: MediaPlayer.Infinite
                    onStopped: visible = false
                }

                Text {
                    anchors.centerIn: parent
                    text: qsTr("点击预览按钮查看效果")
                    color: "#666"
                    font.pixelSize: 14
                    visible: !previewVideo.visible
                }
            }

            ListView {
                id: wallpaperList
                Layout.fillWidth: true
                Layout.preferredHeight: contentHeight
                model: dccData.wallpaperModel
                spacing: 4
                interactive: false

                delegate: Rectangle {
                    width: wallpaperList.width
                    height: 56
                    radius: 6
                    color: modelData.path === dccData.currentVideo ? "#2b5d8a" : "#1e2a3a"
                    border.color: modelData.path === dccData.currentVideo ? "#4a90d9" : "#333"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 10

                        Text {
                            text: modelData.name
                            color: "#ddd"
                            font.pixelSize: 13
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Button {
                            text: qsTr("预览")
                            onClicked: {
                                previewVideo.source = dccData.urlFromPath(modelData.path)
                                previewVideo.visible = true
                                previewVideo.play()
                            }
                        }

                        Button {
                            text: qsTr("应用")
                            enabled: modelData.path !== dccData.currentVideo
                            onClicked: dccData.applyWallpaper(modelData.path)
                        }
                    }
                }
            }
        }
    }
}
