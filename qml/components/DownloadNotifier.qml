// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// How a download ended, said by the platform while the browser is out of sight, where
// the bar over the page would not be seen (docs/DECISIONS/0038-download-controls.md):
// one of Nemo.Notifications' for a download that arrived, and one for a download that
// failed. A tap on the first opens the file, and on the second the list of downloads,
// with the browser brought forward for it. One already shown goes when its download
// starts again, and when it is tapped.
import QtQuick 2.6
import Nemo.Notifications 1.0
import harbour.salama 1.0

QtObject {
    id: notifier

    // By the download's id that lasts.
    property var shown: ({})
    // The browser's own icon, from this file's place in the package, as the pages'
    // notifications have it (components/NotificationCenter.qml).
    readonly property string appIcon:
        Qt.resolvedUrl("../../../icons/hicolor/172x172/apps/harbour-salama.png")

    // The list of downloads, with the browser in front.
    signal downloadsRequested()

    property Component notificationComponent: Component {
        Notification {
            property int downloadId: 0
            property bool arrived: false

            objectName: "downloadNotification"
            appName: "Salama"
            appIcon: notifier.appIcon
            // With no D-Bus call named, a tap is told to this process.
            remoteActions: [{ "name": "default" }]
            onClicked: notifier.activate(downloadId, arrived)
            onClosed: notifier.forget(downloadId)
        }
    }

    function ended(downloadId, status) {
        // Out of sight, as the browsing page tells PageActivity.
        if (!PageActivity.background
                || (status !== DownloadModel.Done && status !== DownloadModel.Failed)) {
            return
        }
        var download = DownloadModel.details(downloadId)
        if (download.name === undefined) {
            return
        }
        close(downloadId)
        var arrived = status === DownloadModel.Done
        var notification = notificationComponent.createObject(notifier, {
            "downloadId": downloadId,
            "arrived": arrived
        })
        notification.summary = arrived ? qsTr("Download finished") : qsTr("Download failed")
        notification.body = download.name
        notification.previewSummary = notification.summary
        notification.previewBody = notification.body
        shown[downloadId] = notification
        notification.publish()
    }

    function activate(downloadId, arrived) {
        var url = arrived ? DownloadModel.fileUrl(DownloadModel.rowOf(downloadId)) : ""
        close(downloadId)
        if (url.length > 0) {
            Qt.openUrlExternally(url)
        } else {
            downloadsRequested()
        }
    }

    function close(downloadId) {
        var notification = shown[downloadId]
        if (notification) {
            delete shown[downloadId]
            notification.close()
            notification.destroy()
        }
    }

    // Swiped away, or closed by the platform.
    function forget(downloadId) {
        var notification = shown[downloadId]
        if (notification) {
            delete shown[downloadId]
            notification.destroy()
        }
    }

    property Connections downloads: Connections {
        target: DownloadModel
        onDownloadEnded: notifier.ended(downloadId, status)
        onDownloadStarted: notifier.close(downloadId)
    }
}
