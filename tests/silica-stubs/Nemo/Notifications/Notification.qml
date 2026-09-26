// Stub of Nemo.Notifications' Notification: the properties salama sets, recording what
// is published and closed, with the platform's side -- a tap, a swipe -- raised by the
// tests through the signals. Names follow nemo-qml-plugin-notifications
// src/notification.h.
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

    // Test hooks: how often it was published, and whether it was closed.
    property int publishCount: 0
    property bool isClosed: false

    // A tap on the notification, and its going: dismissed, expired or closed.
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

    // What the platform still shows of this application's: nothing, on a host.
    function notifications() {
        return []
    }
}
