// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One download: at its head where it stands (DownloadIndicator), with the stop or the
// arrow it is fetched again by; then the file's name, and under it how far along it is,
// how it ended, or -- once it is there -- how large it is and the site it came from
// (docs/DECISIONS/0038-download-controls.md).
//
// A tap opens a file that is there and fetches again one that failed or was stopped;
// the row's menu has the same and the rest: copying where it came from, deleting the
// file, forgetting the row.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: row

    readonly property bool running: model.status === DownloadModel.Running
    readonly property bool done: model.status === DownloadModel.Done
    readonly property bool failed: model.status === DownloadModel.Failed
    readonly property bool stopped: model.status === DownloadModel.Canceled

    signal openRequested()
    signal stopRequested()
    signal retryRequested()
    // By the id that lasts: the list may have moved under the remorse.
    signal deleteRequested(int downloadId)
    signal removeRequested()

    objectName: "downloadDelegate"
    width: ListView.view.width
    contentHeight: Theme.itemSizeMedium
    // What a tap does: the file opened, or fetched again.
    onClicked: {
        if (done && model.fileExists) {
            openRequested()
        } else if ((failed || stopped) && model.retryable) {
            retryRequested()
        }
    }

    menu: ContextMenu {
        MenuItem {
            objectName: "stopDownloadMenu"
            visible: row.running
            text: qsTr("Stop")
            onClicked: row.stopRequested()
        }
        MenuItem {
            objectName: "retryDownloadMenu"
            visible: (row.failed || row.stopped) && model.retryable
            text: qsTr("Retry")
            onClicked: row.retryRequested()
        }
        MenuItem {
            objectName: "openDownloadMenu"
            visible: row.done && model.fileExists
            text: qsTr("Open")
            onClicked: row.openRequested()
        }
        MenuItem {
            objectName: "copyDownloadLinkMenu"
            visible: model.url.length > 0
            text: qsTr("Copy link")
            onClicked: Clipboard.text = model.url
        }
        MenuItem {
            objectName: "deleteDownloadMenu"
            visible: row.done && model.fileExists
            text: qsTr("Delete file")
            onClicked: {
                var downloadId = model.downloadId
                row.remorseAction(qsTr("Deleting file"), function () {
                    row.deleteRequested(downloadId)
                })
            }
        }
        // The files stay where they are; this forgets that they were downloaded. Not for
        // one still coming, which would go on with nothing left to stop it by.
        MenuItem {
            objectName: "removeDownloadMenu"
            visible: !row.running
            text: qsTr("Remove from list")
            onClicked: row.removeRequested()
        }
    }

    DownloadIndicator {
        id: indicator

        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        status: model.status
        progress: model.progress
        fileIcon: model.icon
        retryable: model.retryable
        fileExists: model.fileExists
        actionable: true
        highlighted: row.highlighted
        onClicked: {
            if (row.running) {
                row.stopRequested()
            } else {
                row.retryRequested()
            }
        }
    }

    Column {
        anchors {
            left: indicator.right
            leftMargin: Theme.paddingLarge
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            objectName: "downloadName"
            width: parent.width
            text: model.name
            truncationMode: TruncationMode.Fade
            color: row.highlighted ? Theme.highlightColor : Theme.primaryColor
            opacity: row.done && !model.fileExists ? Theme.opacityHigh : 1.0
        }

        Label {
            objectName: "downloadStatus"
            width: parent.width
            text: {
                var host = SearchSettings.displayAddress(model.url)
                var size = model.size > 0 ? Format.formatFileSize(model.size) : ""
                if (row.running) {
                    return size.length > 0 ? qsTr("%1% of %2").arg(model.progress).arg(size)
                                           : qsTr("Downloading, %1%").arg(model.progress)
                }
                if (row.failed) {
                    return model.retryable ? qsTr("Failed, tap to retry") : qsTr("Failed")
                }
                if (row.stopped) {
                    return model.retryable ? qsTr("Stopped, tap to retry") : qsTr("Stopped")
                }
                if (!model.fileExists) {
                    return qsTr("Moved or deleted")
                }
                return size.length > 0 && host.length > 0 ? size + " · " + host
                                                           : size + host
            }
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: row.failed ? Theme.errorColor
                              : row.highlighted ? Theme.secondaryHighlightColor
                                                : Theme.secondaryColor
        }
    }
}
