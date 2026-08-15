// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import org.deepin.dcc 1.0
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import QtMultimedia

DccObject {
    // ─── 启用开关 ───
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

    // ─── 上传视频 ───
    DccObject {
        name: "memeUpload"
        parentName: "meme"
        displayName: qsTr("上传视频")
        weight: 15
        pageType: DccObject.Item
        backgroundType: DccObject.Normal
        page: ColumnLayout {
            spacing: 8
            Layout.fillWidth: true

            RowLayout {
                spacing: 10

                Button {
                    text: qsTr("选择视频上传")
                    onClicked: fileDialog.open()
                }

                Text {
                    text: qsTr("支持 H264 MP4，其他格式自动转码")
                    color: "#888"
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
            }

            // 转码进度条
            ProgressBar {
                Layout.fillWidth: true
                visible: dccData.converting
                from: 0
                to: 100
                value: dccData.convertProgress
            }

            Text {
                visible: dccData.converting
                text: qsTr("正在转码... %1%").arg(dccData.convertProgress)
                color: "#4a90d9"
                font.pixelSize: 12
            }

            Button {
                visible: dccData.converting
                text: qsTr("取消")
                onClicked: dccData.cancelConvert()
            }

            FileDialog {
                id: fileDialog
                title: qsTr("选择视频文件")
                nameFilters: [qsTr("视频文件 (*.mp4 *.mkv *.webm *.avi *.mov)")]
                onAccepted: dccData.uploadVideo(selectedFile)
            }
        }
    }

    // ─── 预览 + 壁纸列表（合并到同一 DccObject，previewVideo id 可在 delegate 中引用）───
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

            // ── 预览区域 ──
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
                    onPlaying: visible = true
                }

                Text {
                    anchors.centerIn: parent
                    text: qsTr("点击列表中的预览按钮查看效果")
                    color: "#666"
                    font.pixelSize: 14
                    visible: !previewVideo.visible
                }
            }

            // ── 壁纸网格 ──
            GridView {
                id: wallpaperGrid
                Layout.fillWidth: true
                Layout.preferredHeight: Math.ceil(count / 3) * 130 + 20
                model: dccData.wallpaperModel
                cellWidth: width / 3 - 4
                cellHeight: 130
                interactive: false
                clip: true

                delegate: Rectangle {
                    width: wallpaperGrid.cellWidth - 4
                    height: wallpaperGrid.cellHeight - 4
                    radius: 6
                    color: model.path === dccData.currentVideo ? "#2b5d8a" : "#1e2a3a"
                    border.color: model.path === dccData.currentVideo ? "#4a90d9" : "#333"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        // 缩略图占位
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 50
                            color: model.isPreset ? "#3a5068" : "#5a4a3a"
                            radius: 4

                            Text {
                                anchors.centerIn: parent
                                text: model.isPreset ? qsTr("预置") : qsTr("上传")
                                color: "#ccc"
                                font.pixelSize: 11
                            }
                        }

                        Text {
                            text: model.name
                            color: "#ddd"
                            font.pixelSize: 11
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        RowLayout {
                            spacing: 4

                            Button {
                                text: qsTr("预览")
                                font.pixelSize: 10
                                onClicked: {
                                    previewVideo.source = dccData.urlFromPath(model.path)
                                    previewVideo.play()
                                }
                            }

                            Button {
                                text: qsTr("应用")
                                enabled: model.path !== dccData.currentVideo
                                font.pixelSize: 10
                                onClicked: dccData.applyWallpaper(model.path)
                            }

                            Button {
                                text: qsTr("删除")
                                visible: !model.isPreset
                                font.pixelSize: 10
                                onClicked: dccData.removeUserWallpaper(index)
                            }
                        }
                    }
                }
            }

            Text {
                visible: wallpaperGrid.count === 0
                text: qsTr("暂无壁纸，请上传视频")
                color: "#666"
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }
}
