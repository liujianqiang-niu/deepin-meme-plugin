// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtMultimedia

// 预览窗口: 循环播放壁纸 + 按钮演示各特效
Rectangle {
    id: previewRoot
    property string themeId: ""
    property bool enabled: false
    signal previewEffectRequested(string effectType)

    color: "#1a1a1a"
    implicitHeight: 280
    implicitWidth: 480

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // 视频预览区
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "black"
            radius: 8

            Video {
                id: videoPlayer
                anchors.fill: parent
                anchors.margins: 2
                source: previewRoot.enabled && previewRoot.themeId !== ""
                        ? dccData.previewVideoUrl(previewRoot.themeId)
                        : ""
                loops: MediaPlayer.Infinite
                muted: true
                fillMode: VideoOutput.PreserveAspectCrop
            }

            // 未启用提示
            Text {
                anchors.centerIn: parent
                visible: !previewRoot.enabled
                text: qsTr("请先启用趣味壁纸并选择主题")
                color: "#888"
            }
        }

        // 特效演示按钮组
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

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
                    Layout.fillWidth: true
                    onClicked: {
                        previewRoot.previewEffectRequested(modelData.type)
                    }
                }
            }
        }
    }
}
