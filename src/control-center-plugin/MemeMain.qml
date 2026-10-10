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

    // ─── 解码方式 ───
    DccObject {
        name: "memeDecodeMode"
        parentName: "meme"
        displayName: qsTr("解码方式")
        description: qsTr("自动：依次尝试 VAAPI→CUDA→Vulkan→DRM，命中即用；VAAPI 适合 Intel/AMD 及多数国产卡，CUDA 适合 NVIDIA，Vulkan 适合跨厂商/国产卡，软件最兼容但最费 CPU")
        weight: 11
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: ComboBox {
            property var vals: ["auto", "vaapi", "vulkan", "cuda", "software"]
            model: [qsTr("自动(推荐)"), "VAAPI", "Vulkan", "CUDA", qsTr("软件")]
            currentIndex: Math.max(0, vals.indexOf(dccData.decodeMode))
            onActivated: dccData.decodeMode = vals[currentIndex]
        }
    }

    // ─── 最大化窗口时也暂停 ───
    DccObject {
        name: "memePauseMax"
        parentName: "meme"
        displayName: qsTr("最大化窗口时也暂停")
        description: qsTr("关闭：仅全屏窗口(完全遮挡)才暂停；开启：任何最大化窗口也暂停（半透明窗口其实仍能看到壁纸）")
        weight: 12
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: Switch {
            checked: dccData.pauseOnMaximized
            onCheckedChanged: dccData.pauseOnMaximized = checked
        }
    }

    // ─── 帧率上限（省电）───
    DccObject {
        name: "memeFpsCap"
        parentName: "meme"
        displayName: qsTr("帧率上限")
        description: qsTr("限制壁纸刷新帧率以降低 CPU/GPU 占用；「跟随源」= 不限制")
        weight: 13
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: ComboBox {
            property var vals: [0, 24, 30, 60]
            model: [qsTr("跟随源"), "24", "30", "60"]
            currentIndex: Math.max(0, vals.indexOf(dccData.fpsCap))
            onActivated: dccData.fpsCap = vals[currentIndex]
        }
    }

    // ─── 渲染宽度（省电）───
    DccObject {
        name: "memeRenderWidth"
        parentName: "meme"
        displayName: qsTr("渲染宽度")
        description: qsTr("把壁纸下采样到该宽度以降低 CPU/GPU 占用；「屏幕宽度」= 原始清晰度")
        weight: 14
        pageType: DccObject.Editor
        backgroundType: DccObject.Normal
        page: ComboBox {
            property var vals: [0, 1920, 1280, 960, 854]
            model: [qsTr("屏幕宽度"), "1920", "1280", "960", "854"]
            currentIndex: Math.max(0, vals.indexOf(dccData.renderWidth))
            onActivated: dccData.renderWidth = vals[currentIndex]
        }
    }

    // ─── 上传视频 + 进度 ───
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
                    enabled: !dccData.converting
                    onClicked: fileDialog.open()
                }

                Text {
                    text: qsTr("支持 H264 / HEVC 的 MP4 直接入库；其他格式自动转码为 H264")
                    color: "#888"
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
            }

            // 进度条（转码时显示）
            ProgressBar {
                Layout.fillWidth: true
                visible: dccData.converting
                from: 0
                to: 100
                value: dccData.convertProgress
            }

            // 进度文字
            RowLayout {
                visible: dccData.converting
                spacing: 8

                Text {
                    text: qsTr("正在转码... %1%").arg(dccData.convertProgress)
                    color: "#4a90d9"
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }

                Button {
                    text: qsTr("取消")
                    onClicked: dccData.cancelConvert()
                }
            }

            // 状态消息（上传完成/失败/已删除等）
            Text {
                visible: dccData.statusMessage.length > 0
                text: dccData.statusMessage
                color: dccData.converting ? "#4a90d9" : "#66a0d0"
                font.pixelSize: 12
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            FileDialog {
                id: fileDialog
                title: qsTr("选择视频文件")
                nameFilters: [qsTr("视频文件 (*.mp4 *.mkv *.webm *.avi *.mov)")]
                onAccepted: dccData.uploadVideo(selectedFile)
            }
        }
    }

    // ─── 预览 + 壁纸列表（合并到同一 DccObject）───
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
                    text: qsTr("点击列表中的「预览」按钮查看效果")
                    color: "#666"
                    font.pixelSize: 14
                    visible: !previewVideo.visible
                }
            }

            // ── 壁纸网格 ──
            GridView {
                id: wallpaperGrid
                Layout.fillWidth: true
                Layout.preferredHeight: Math.ceil(count / 3) * 140 + 20
                model: dccData.wallpaperModel
                cellWidth: width / 3 - 4
                cellHeight: 140
                interactive: false
                clip: true

                delegate: Rectangle {
                    width: wallpaperGrid.cellWidth - 4
                    height: wallpaperGrid.cellHeight - 4
                    radius: 6
                    clip: true
                    color: model.path === dccData.currentVideo ? "#2b5d8a" : "#1e2a3a"
                    border.color: model.path === dccData.currentVideo ? "#4a90d9" : "#333"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

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
                            Layout.fillWidth: true

                            Button {
                                text: qsTr("预览")
                                font.pixelSize: 10
                                Layout.fillWidth: true
                                onClicked: {
                                    previewVideo.source = dccData.urlFromPath(model.path)
                                    previewVideo.play()
                                }
                            }

                            Button {
                                text: qsTr("应用")
                                enabled: model.path !== dccData.currentVideo
                                font.pixelSize: 10
                                Layout.fillWidth: true
                                onClicked: dccData.applyWallpaper(model.path)
                            }

                            Button {
                                text: qsTr("删除")
                                visible: !model.isPreset
                                font.pixelSize: 10
                                Layout.fillWidth: true
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
