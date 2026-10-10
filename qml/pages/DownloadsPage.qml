// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Own list: Harbour apps may not open platform transfers list.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: downloadsPage

    objectName: "downloadsPage"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask

    function open(row, openable) {
        if (openable) {
            Qt.openUrlExternally(DownloadModel.fileUrl(row))
        }
    }

    function act(row, status) {
        if (status === DownloadModel.Running) {
            DownloadModel.pause(row)
        } else {
            DownloadModel.resume(row)
        }
    }

    // File may have been deleted elsewhere meanwhile.
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

            // Files stay; only forgets records.
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
            // Looked up by id at timeout: rows above may have changed.
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
