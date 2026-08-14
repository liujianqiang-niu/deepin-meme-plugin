import QtQuick 2.15
import QtMultimedia

Item {
    property var videoSink: null

    Video {
        id: videoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        muted: true
    }

    // 当 videoSink 设置后,绑定到 Video 的 videoSink
    onVideoSinkChanged: {
        if (videoSink) {
            videoOutput.videoSink = videoSink
        }
    }
}
