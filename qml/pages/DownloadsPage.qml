// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What pages have downloaded, newest first, each with where it stands and what can be
// done with it: one still coming stopped, one that failed or was stopped fetched again,
// one that arrived opened with whatever the platform opens that kind of file with, or
// deleted (docs/DECISIONS/0038-download-controls.md). The list is the browser's own,
// not the platform's list of transfers, which a Harbour application may not open
// (docs/DECISIONS/0022-downloads-list.md). The pulley opens the folder they are saved in.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: downloadsPage

    objectName: "downloadsPage"
    allowedOrientations: Orientation.Portrait

    // Only a download that has arrived has a file to open.
    function open(row) {
        var url = DownloadModel.fileUrl(row)
        if (url.length > 0) {
            Qt.openUrlExternally(url)
        }
    }

    // The files may have been moved or deleted while the list was out of sight.
    onStatusChanged: {
        if (status === PageStatus.Activating) {
            DownloadModel.refresh()
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
            MenuItem {
                objectName: "openDownloadFolderMenu"
                text: qsTr("Open folder")
                onClicked: Qt.openUrlExternally(DownloadModel.directoryUrl)
            }
            // The files stay where they are; this forgets that they were downloaded.
            // Those still coming stay.
            MenuItem {
                objectName: "clearDownloadsMenu"
                text: qsTr("Clear list")
                enabled: DownloadModel.count > DownloadModel.runningCount
                onClicked: DownloadModel.clear()
            }
        }

        delegate: DownloadDelegate {
            onOpenRequested: downloadsPage.open(index)
            onStopRequested: DownloadModel.stop(index)
            onRetryRequested: DownloadModel.retry(index)
            onDeleteRequested: DownloadModel.deleteFile(DownloadModel.rowOf(downloadId))
            onRemoveRequested: DownloadModel.remove(index)
        }

        ViewPlaceholder {
            enabled: DownloadModel.count === 0
            text: qsTr("No downloads")
            hintText: qsTr("Files downloaded from pages are listed here")
        }

        VerticalScrollDecorator {}
    }
}
