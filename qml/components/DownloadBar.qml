// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The bar that comes up over the foot of the browsing page as a download starts, so
// that what was asked for is seen to be coming without going to the list for it
// (docs/DECISIONS/0038-download-controls.md). It names the newest download, with
// where it stands at its head as the list has it (DownloadIndicator):
//
//  * while it comes, how far along it is -- or how far along all those still coming
//    are, together, while there are more -- and a close that puts the bar away and
//    leaves the download going, with the ring round the menu button to say so;
//  * arrived, that it has, and Open, for a while before it goes;
//  * failed, that it did, and Retry where it can be fetched again, for a while longer;
//  * stopped, nothing: it was stopped on purpose.
//
// A tap anywhere else on it is the list of downloads. It stays out of the way of what
// is over the page -- the menu sheet, the address being edited, the find bar, the
// grid -- and comes back with them gone.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

BackgroundItem {
    id: bar

    // The download shown, by the id that lasts, and its roles (DownloadModel.details()).
    property int downloadId: 0
    property var download: ({})
    property bool dismissed: true
    // Something else is over the foot of the page.
    property bool covered: false
    // How long the end of a download is shown, in milliseconds.
    property int arrivedTimeout: 5000
    property int failedTimeout: 10000

    readonly property bool shown: downloadId > 0 && !dismissed && download.status !== undefined
    readonly property bool running: download.status === DownloadModel.Running
    readonly property bool arrived: download.status === DownloadModel.Done
    readonly property bool failed: download.status === DownloadModel.Failed
    readonly property bool together: running && DownloadModel.runningCount > 1
    // What the button at its end does: "open", "retry", or "close".
    readonly property string action: arrived && download.fileExists ? "open"
                                     : failed && download.retryable ? "retry" : "close"

    objectName: "downloadBar"
    height: Theme.itemSizeMedium
    enabled: shown && !covered
    visible: opacity > 0
    opacity: enabled ? 1.0 : 0.0
    highlightedColor: "transparent"
    // The list, over the browsing page, as the menu opens it.
    onClicked: pageStack.push(Qt.resolvedUrl("../pages/DownloadsPage.qml"))

    Behavior on opacity {
        FadeAnimation {}
    }

    function refresh() {
        download = downloadId > 0 ? DownloadModel.details(downloadId) : ({})
    }

    function started(id) {
        hideTimer.stop()
        downloadId = id
        dismissed = false
        refresh()
    }

    function ended(id, status) {
        if (id !== downloadId) {
            return
        }
        refresh()
        if (status === DownloadModel.Canceled) {
            dismissed = true
            return
        }
        // Put away while it came, and shown again for how it ended.
        dismissed = false
        hideTimer.interval = status === DownloadModel.Done ? arrivedTimeout : failedTimeout
        hideTimer.restart()
    }

    function dismiss() {
        hideTimer.stop()
        dismissed = true
    }

    function act() {
        var row = DownloadModel.rowOf(downloadId)
        if (action === "open") {
            Qt.openUrlExternally(DownloadModel.fileUrl(row))
            dismiss()
        } else if (action === "retry") {
            DownloadModel.retry(row)
        } else {
            dismiss()
        }
    }

    Connections {
        target: DownloadModel
        onDownloadStarted: bar.started(downloadId)
        onDownloadEnded: bar.ended(downloadId, status)
        // How far along it is, and whether it is still in the list at all.
        onDataChanged: bar.refresh()
        onRowsRemoved: bar.refresh()
    }

    Timer {
        id: hideTimer

        objectName: "downloadBarTimer"
        onTriggered: bar.dismissed = true
    }

    // The ground the menu sheet has, rounded, as something laid over the page rather
    // than a part of it.
    Rectangle {
        objectName: "downloadBarBackground"
        anchors.fill: parent
        radius: Theme.paddingMedium
        color: Theme.highlightDimmerColor
        border {
            width: Theme._lineWidth
            color: Theme.rgba(Theme.highlightColor, Theme.opacityFaint)
        }
    }

    DownloadIndicator {
        id: indicator

        anchors {
            left: parent.left
            leftMargin: Theme.paddingLarge
            verticalCenter: parent.verticalCenter
        }
        status: bar.download.status !== undefined ? bar.download.status
                                                  : DownloadModel.Running
        progress: bar.together ? DownloadModel.runningProgress
                               : (bar.download.progress !== undefined ? bar.download.progress : 0)
        fileIcon: bar.download.icon !== undefined ? bar.download.icon
                                                  : "image://theme/icon-m-file-other"
        retryable: bar.download.retryable === true
        fileExists: bar.download.fileExists === true
        highlighted: bar.highlighted
    }

    Column {
        anchors {
            left: indicator.right
            leftMargin: Theme.paddingLarge
            right: actionButton.left
            rightMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }

        Label {
            objectName: "downloadBarName"
            width: parent.width
            text: bar.download.name !== undefined ? bar.download.name : ""
            truncationMode: TruncationMode.Fade
            color: bar.highlighted ? Theme.highlightColor : Theme.primaryColor
        }

        Label {
            objectName: "downloadBarStatus"
            width: parent.width
            text: {
                if (bar.together) {
                    return qsTr("%1 downloads, %2%").arg(DownloadModel.runningCount)
                                                    .arg(DownloadModel.runningProgress)
                }
                if (bar.running) {
                    return bar.download.size > 0
                            ? qsTr("%1% of %2").arg(bar.download.progress)
                                               .arg(Format.formatFileSize(bar.download.size))
                            : qsTr("Downloading, %1%").arg(bar.download.progress)
                }
                if (bar.arrived) {
                    return qsTr("Downloaded")
                }
                return bar.failed ? qsTr("Failed") : ""
            }
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: bar.failed ? Theme.errorColor
                              : bar.highlighted ? Theme.secondaryHighlightColor
                                                : Theme.secondaryColor
        }
    }

    // Open and Retry in words, in the highlight colour, as a notice's action is; the
    // close as an icon.
    BackgroundItem {
        id: actionButton

        objectName: "downloadBarAction"
        anchors {
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        width: bar.action === "close" ? height
                                      : actionLabel.implicitWidth + 2 * Theme.paddingLarge
        highlightedColor: "transparent"
        onClicked: bar.act()

        Label {
            id: actionLabel

            objectName: "downloadBarActionLabel"
            anchors.centerIn: parent
            visible: bar.action !== "close"
            text: bar.action === "open" ? qsTr("Open") : qsTr("Retry")
            font.pixelSize: Theme.fontSizeSmall
            color: actionButton.highlighted ? Theme.primaryColor : Theme.highlightColor
        }

        Icon {
            objectName: "downloadBarClose"
            anchors.centerIn: parent
            visible: bar.action === "close"
            source: "image://theme/icon-m-clear"
            highlighted: actionButton.highlighted
        }
    }
}
