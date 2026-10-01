// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One download: at its start a ring that fills as it comes, the way the menu's
// Downloads and the cover show the downloads together, with what a tap on it does in
// its middle -- pause one coming, resume one paused, try one that failed again -- or,
// once the file is there, the icon of its kind. Beside it the file's name, and under
// that how far along it is, how it stopped, or the size and the site it came from
// (docs/DECISIONS/0038-download-status.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: row

    readonly property bool running: model.status === DownloadModel.Running
    readonly property bool done: model.status === DownloadModel.Done
    readonly property bool failed: model.status === DownloadModel.Failed
    readonly property bool openable: done && model.fileExists

    // The ring's tap: pause or resume.
    signal actionRequested()
    signal removeRequested()
    signal deleteRequested()
    signal folderRequested()

    objectName: "downloadDelegate"
    width: ListView.view.width
    contentHeight: Theme.itemSizeMedium
    menu: ContextMenu {
        MenuItem {
            objectName: "openDownloadMenu"
            visible: row.openable
            text: qsTr("Open")
            onClicked: row.clicked()
        }
        MenuItem {
            objectName: "pauseDownloadMenu"
            visible: row.running
            text: qsTr("Pause")
            onClicked: row.actionRequested()
        }
        MenuItem {
            objectName: "resumeDownloadMenu"
            visible: !row.running && !row.done
            text: {
                if (!model.resumable) {
                    //: Fetch again a download the engine has forgotten, from the start
                    return qsTr("Download again")
                }
                return row.failed ? qsTr("Retry") : qsTr("Resume")
            }
            onClicked: row.actionRequested()
        }
        MenuItem {
            objectName: "openFolderMenu"
            visible: row.openable
            text: qsTr("Open folder")
            onClicked: row.folderRequested()
        }
        MenuItem {
            objectName: "copyDownloadLinkMenu"
            text: qsTr("Copy link")
            onClicked: Clipboard.text = model.url
        }
        MenuItem {
            objectName: "deleteDownloadMenu"
            visible: row.openable
            text: qsTr("Delete file")
            onClicked: row.deleteRequested()
        }
        MenuItem {
            objectName: "removeDownloadMenu"
            text: qsTr("Remove from list")
            onClicked: row.removeRequested()
        }
    }

    DownloadText {
        id: says
    }

    Item {
        id: badge

        objectName: "downloadBadge"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin - Theme.paddingSmall
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium + Theme.paddingSmall
        height: width

        // Silica's ring, as the menu draws it: lit while coming, dimmed while paused,
        // and in the error colour once failed, holding as far as it had got.
        ProgressCircle {
            objectName: "downloadProgress"
            anchors.centerIn: parent
            width: Theme.iconSizeMedium
            height: width
            visible: !row.done
            value: model.progress / 100
            progressColor: row.failed ? Theme.errorColor
                                      : row.running ? Theme.highlightColor
                                                    : Theme.secondaryHighlightColor
            backgroundColor: Theme.rgba(row.failed ? Theme.errorColor : Theme.primaryColor,
                                        Theme.opacityFaint)
            borderWidth: Theme.paddingSmall / 2
        }

        IconButton {
            objectName: "downloadAction"
            anchors.fill: parent
            visible: !row.done
            icon.source: row.running ? "image://theme/icon-m-pause"
                                     : row.failed || !model.resumable ? "image://theme/icon-m-refresh"
                                                                      : "image://theme/icon-m-play"
            icon.sourceSize: Qt.size(Theme.iconSizeSmallPlus, Theme.iconSizeSmallPlus)
            onClicked: row.actionRequested()
        }

        Icon {
            objectName: "downloadFileIcon"
            anchors.centerIn: parent
            visible: row.done
            source: says.fileIcon(model.mimeType, model.name)
            opacity: model.fileExists ? 1.0 : Theme.opacityLow
            highlighted: row.highlighted
        }
    }

    Column {
        anchors {
            left: badge.right
            leftMargin: Theme.paddingMedium
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
        }

        Label {
            objectName: "downloadStatus"
            width: parent.width
            text: says.status(model)
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: row.failed ? Theme.errorColor
                              : row.highlighted ? Theme.secondaryHighlightColor
                                                : Theme.secondaryColor
        }
    }
}
