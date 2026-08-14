import QtQuick 2.15
import QtMultimedia

Item {
    property string videoSource: ""

    Video {
        id: videoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectCrop
        muted: true
        loops: MediaPlayer.Infinite
        source: videoSource !== "" ? Qt.resolvedUrl("file://" + videoSource) : ""
        autoPlay: true
    }
}
