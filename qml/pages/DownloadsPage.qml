// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What pages have downloaded, newest first. Each row's ring pauses one coming and
// resumes one paused or failed; a tap on one that has arrived opens the file with
// whatever the platform opens that kind of file with, and its menu deletes it. The
// list is the browser's own, not the platform's list of
// transfers, which a Harbour application may not open
// (docs/DECISIONS/0022-downloads-list.md, 0038-download-status.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: downloadsPage

    objectName: "downloadsPage"
    allowedOrientations: Orientation.Portrait

    // Only a download that has arrived, and whose file is still there, has a file to
    // open.
    function open(row, openable) {
        if (openable) {
            Qt.openUrlExternally(DownloadModel.fileUrl(row))
        }
    }

    // The ring's tap, and the menu's Pause and Resume.
    function act(row, status) {
        if (status === DownloadModel.Running) {
            DownloadModel.pause(row)
        } else {
            DownloadModel.resume(row)
        }
    }

    // A file may have been deleted elsewhere while the page was away.
    onStatusChanged: {
        if (status === PageStatus.Activating) {
            DownloadModel.refreshFiles()
        }
    }

    SilicaListView {
        id: downloadList

        objectName: "downloadList"
        anchors.fill: parent
        model: DownloadModel
        header: PageHeader {
            title: qsTr("Downloads")
        }

        PullDownMenu {
            objectName: "downloadsPulley"

            // The files stay where they are; this forgets that they were downloaded.
            MenuItem {
                objectName: "clearFinishedDownloadsMenu"
                text: qsTr("Clear finished")
                enabled: DownloadModel.count > 0
                onClicked: DownloadModel.clearFinished()
            }
        }

        delegate: DownloadDelegate {
            id: delegate

            onClicked: downloadsPage.open(index, delegate.openable)
            onActionRequested: downloadsPage.act(index, model.status)
            onRemoveRequested: DownloadModel.remove(index)
            // Found again by its id when the time is up: rows above it may have come
            // or gone in the meantime.
            onDeleteRequested: {
                var downloadId = model.downloadId
                //: The few seconds to change one's mind before a downloaded file is deleted
                delegate.remorseAction(qsTr("Deleting"), function () {
                    DownloadModel.deleteFile(DownloadModel.rowOf(downloadId))
                })
            }
        }

        ViewPlaceholder {
            enabled: DownloadModel.count === 0
            text: qsTr("No downloads")
            hintText: qsTr("Files downloaded from pages are listed here")
        }

        VerticalScrollDecorator {}
    }
}
