// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtMultimedia

Rectangle {
    id: previewRoot
    property string themeId: ""
    property bool enabled: false
    property string wallpaperUrl: ""
    signal previewEffectRequested(string effectType)

    function playEffect(url) {
        if (!url || url === "") return
        effectVideo.source = "file://" + url
        effectVideo.visible = true
        effectVideo.play()
    }

    color: "#0d1117"
    implicitHeight: 320
    implicitWidth: 480
    clip: true

    Rectangle {
        id: miniDesktop
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height - 50
        anchors.margins: 8
        radius: 6
        color: "#1a1a2e"
        clip: true

        Image {
            id: wallpaperImage
            anchors.fill: parent
            source: previewRoot.enabled && previewRoot.wallpaperUrl !== ""
                    ? "file://" + previewRoot.wallpaperUrl : ""
            fillMode: Image.PreserveAspectCrop
            visible: previewRoot.enabled && previewRoot.wallpaperUrl !== ""
        }

        Text {
            anchors.centerIn: parent
            text: previewRoot.enabled ? qsTr("选择主题后显示壁纸预览") : qsTr("请先启用趣味壁纸特效")
            color: "#666"
            font.pixelSize: 14
            visible: !wallpaperImage.visible
        }

        GridLayout {
            anchors.fill: parent
            anchors.margins: 16
            columns: 3
            rowSpacing: 16
            columnSpacing: 16
            visible: previewRoot.enabled

            Repeater {
                model: [
                    { name: "文档.txt", icon: "📄" },
                    { name: "图片.png", icon: "🖼" },
                    { name: "音乐.mp3", icon: "🎵" },
                    { name: "视频.mp4", icon: "🎬" },
                    { name: "文件夹", icon: "📁" },
                    { name: "代码.py", icon: "💻" }
                ]
                delegate: ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 2

                    property bool destroyed: false

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 48; height: 48
                        radius: 6
                        color: "#ffffff15"
                        border.color: "#ffffff30"
                        border.width: 1
                        opacity: destroyed ? 0 : 1

                        Text {
                            anchors.centerIn: parent
                            text: modelData.icon
                            font.pixelSize: 28
                        }

                        Behavior on opacity { NumberAnimation { duration: 400 } }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData.name
                        color: "#ddd"
                        font.pixelSize: 9
                        opacity: destroyed ? 0 : 1
                        Behavior on opacity { NumberAnimation { duration: 400 } }
                    }

                    Connections {
                        target: previewRoot
                        function onPreviewEffectRequested(effectType) {
                            if (effectType === "delete" && index === 0)
                                destroyed = true
                            else if (effectType === "create" && index === 0)
                                destroyed = false
                        }
                    }
                }
            }
        }

        Video {
            id: effectVideo
            anchors.fill: parent
            muted: false
            fillMode: VideoOutput.PreserveAspectFit
            visible: false
            z: 50
            onStopped: {
                visible = false
            }
        }
    }

    RowLayout {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 6
        spacing: 6

        Repeater {
            model: [
                { label: qsTr("删除"), type: "delete" },
                { label: qsTr("新建"), type: "create" },
                { label: qsTr("重命名"), type: "rename" },
                { label: qsTr("移动"), type: "move" },
                { label: qsTr("复制"), type: "copy" }
            ]
            delegate: Button {
                text: modelData.label
                enabled: previewRoot.enabled
                Layout.preferredWidth: 80
                onClicked: previewRoot.previewEffectRequested(modelData.type)
            }
        }
    }
}
