// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

ListItem {
    id: row

    readonly property bool running: model.status === DownloadModel.Running
    readonly property bool done: model.status === DownloadModel.Done
    readonly property bool failed: model.status === DownloadModel.Failed
    readonly property bool openable: done && model.fileExists

    signal actionRequested()
    signal removeRequested()
    signal deleteRequested()

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
