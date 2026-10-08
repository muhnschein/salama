// Stub of Nemo.Notifications Notification: records publish/close; tests raise platform side
// (tap, swipe) via signals. Names follow nemo-qml-plugin-notifications src/notification.h.
import QtQuick 2.6

QtObject {
    property string appName
    property string appIcon
    property string summary
    property string body
    property string previewSummary
    property string previewBody
    property string subText
    property string icon
    property var remoteActions: []
    property int replacesId: 0

    property int publishCount: 0
    property bool isClosed: false

    signal clicked()
    signal closed(int reason)

    function publish() {
        publishCount += 1
        if (replacesId === 0) {
            replacesId = publishCount
        }
    }

    function close() {
        isClosed = true
    }

    // Platform's shown notifications for app: none on host.
    function notifications() {
        return []
    }
}
