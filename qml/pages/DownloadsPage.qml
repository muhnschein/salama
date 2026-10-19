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
    allowedOrientations: Orientation.Portrait

    // By id: list shows search's rows, not DownloadModel's.
    function open(downloadId, openable) {
        if (openable) {
            Qt.openUrlExternally(DownloadModel.fileUrl(DownloadModel.rowOf(downloadId)))
        }
    }

    function act(downloadId, status) {
        var row = DownloadModel.rowOf(downloadId)
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

    DownloadSearch {
        id: search

        downloads: DownloadModel
    }

    // As History: flickable owns pulley so it opens above heading; field stays out of list.
    SilicaFlickable {
        objectName: "downloadsPulleyHost"
        anchors.fill: parent
        contentWidth: width
        contentHeight: height

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

        Column {
            id: heading

            objectName: "downloadsHeading"
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }

            PageHeader {
                title: qsTr("Downloads")
            }

            SearchField {
                objectName: "downloadsSearch"
                width: parent.width
                placeholderText: qsTr("Search")
                onTextChanged: search.searchTerm = text
            }
        }

        SilicaListView {
            id: downloadList

            objectName: "downloadList"
            anchors {
                top: heading.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            clip: true
            model: search

            // In progress first, then by start.
            section.property: "dateGroup"
            section.delegate: DateSectionHeader {
                group: Number(section)
            }

            delegate: DownloadDelegate {
                id: delegate

                searching: search.searchTerm.length > 0
                onClicked: downloadsPage.open(model.downloadId, delegate.openable)
                onActionRequested: downloadsPage.act(model.downloadId, model.status)
                onRemoveRequested: DownloadModel.remove(DownloadModel.rowOf(model.downloadId))
                // Looked up at timeout: rows above may have changed.
                onDeleteRequested: {
                    var downloadId = model.downloadId
                    //: The few seconds to change one's mind before a downloaded file is deleted
                    delegate.remorseAction(qsTr("Deleting"), function () {
                        DownloadModel.deleteFile(DownloadModel.rowOf(downloadId))
                    })
                }
            }

            ViewPlaceholder {
                objectName: "downloadsPlaceholder"
                enabled: search.count === 0
                text: search.searchTerm.length > 0 ? qsTr("No matches") : qsTr("No downloads")
                hintText: search.searchTerm.length > 0
                          ? "" : qsTr("Files downloaded from pages are listed here")
            }

            VerticalScrollDecorator {}
        }
    }
}
