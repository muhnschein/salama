// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One download: the file's name, and under it how far along it is, how it ended, or --
// once it is there -- the site it came from. A ring beside it, of Silica's, fills as a
// download still coming goes along, and stays where it is while one is paused; a line
// along its foot would be read as the page's own, which the navigation bar draws in the
// same place. Its menu pauses, resumes, cancels or removes it, or deletes the file
// itself (docs/DECISIONS/0038-download-notice.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: row

    signal removeRequested()

    objectName: "downloadDelegate"
    width: ListView.view.width
    contentHeight: Theme.itemSizeMedium
    menu: ContextMenu {
        // Pausing is the engine's cancel, which keeps the partial file, so resuming can
        // start again from there. A download that has arrived has no engine to ask.
        MenuItem {
            objectName: "pauseDownloadMenu"
            text: qsTr("Pause")
            enabled: model.status === DownloadModel.Running
            onClicked: DownloadModel.pause(model.downloadId)
        }

        MenuItem {
            objectName: "resumeDownloadMenu"
            text: qsTr("Resume")
            enabled: model.status === DownloadModel.Paused
            onClicked: DownloadModel.resume(model.downloadId)
        }

        MenuItem {
            objectName: "cancelDownloadMenu"
            text: qsTr("Cancel")
            enabled: model.status === DownloadModel.Running
                     || model.status === DownloadModel.Paused
            onClicked: DownloadModel.cancel(model.downloadId)
        }

        MenuItem {
            objectName: "deleteDownloadFileMenu"
            text: qsTr("Delete file")
            enabled: model.status === DownloadModel.Done
                     && DownloadModel.hasFile(model.downloadId)
            onClicked: DownloadModel.deleteFile(model.downloadId)
        }

        MenuItem {
            objectName: "removeDownloadMenu"
            text: qsTr("Remove from list")
            onClicked: row.removeRequested()
        }
    }

    Column {
        id: labels

        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            // The ring's room is its own while it is there.
            right: progress.visible ? progress.left : parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            objectName: "downloadName"
            width: parent.width
            text: model.name
            truncationMode: TruncationMode.Fade
            // Paused and cancelled are over, for now or for good, and read as such.
            color: row.highlighted ? Theme.highlightColor
                   : model.status === DownloadModel.Failed ? Theme.errorColor
                   : model.status === DownloadModel.Paused
                     || model.status === DownloadModel.Canceled ? Theme.secondaryColor
                                                                : Theme.primaryColor
        }

        Label {
            objectName: "downloadStatus"
            width: parent.width
            text: {
                if (model.status === DownloadModel.Running) {
                    //: The figure is the share of the file that has arrived
                    return qsTr("Downloading, %1%").arg(model.progress)
                }
                if (model.status === DownloadModel.Paused) {
                    return qsTr("Paused at %1%").arg(model.progress)
                }
                if (model.status === DownloadModel.Failed) {
                    return qsTr("Failed")
                }
                if (model.status === DownloadModel.Canceled) {
                    return qsTr("Cancelled")
                }
                return SearchSettings.displayAddress(model.url)
            }
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: row.highlighted ? Theme.secondaryHighlightColor
                   : model.status === DownloadModel.Failed ? Theme.errorColor
                   : model.status === DownloadModel.Paused ? Theme.highlightColor
                                                           : Theme.secondaryColor
        }
    }

    // How far along, while it is coming; frozen where it was once it is paused.
    ProgressCircle {
        id: progress

        objectName: "downloadProgress"
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        value: model.progress / 100
        visible: model.status === DownloadModel.Running
                 || model.status === DownloadModel.Paused
        progressColor: Theme.highlightColor
        backgroundColor: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
        borderWidth: Theme.paddingSmall / 2
    }
}