// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What pages have downloaded, newest first, with how far along the downloads still
// coming are. A tap on a finished one opens the file with whatever the platform opens
// that kind of file with. The list is the browser's own, not the platform's list of
// transfers, which a Harbour application may not open
// (docs/DECISIONS/0022-downloads-list.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: downloadsPage

    objectName: "downloadsPage"
    allowedOrientations: Orientation.Portrait

    // Only a download that has arrived has a file to open.
    function open(row, status) {
        if (status === DownloadModel.Done) {
            Qt.openUrlExternally(DownloadModel.fileUrl(row))
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

        // The files stay where they are; this forgets that they were downloaded.
        PullDownMenu {
            MenuItem {
                objectName: "clearDownloadsMenu"
                text: qsTr("Clear list")
                enabled: DownloadModel.count > 0
                onClicked: DownloadModel.clear()
            }
        }

        delegate: DownloadDelegate {
            onClicked: downloadsPage.open(index, model.status)
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
