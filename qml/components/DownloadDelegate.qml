// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One download: the file's name, and under it how far along it is, how it ended, or --
// once it is there -- the site it came from. A line along its foot says how far along
// a download still coming is at a glance, as the navigation bar's does for a page.
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
        MenuItem {
            objectName: "removeDownloadMenu"
            text: qsTr("Remove from list")
            onClicked: row.removeRequested()
        }
    }

    Column {
        anchors {
            left: parent.left
            right: parent.right
            margins: Theme.horizontalPageMargin
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
            text: {
                if (model.status === DownloadModel.Running) {
                    return qsTr("Downloading, %1%").arg(model.progress)
                }
                if (model.status === DownloadModel.Failed) {
                    return qsTr("Failed")
                }
                if (model.status === DownloadModel.Canceled) {
                    return qsTr("Cancelled")
                }
                return Settings.displayAddress(model.url)
            }
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: row.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
        }
    }

    Rectangle {
        objectName: "downloadProgress"
        anchors {
            left: parent.left
            bottom: parent.bottom
        }
        height: Theme.paddingSmall
        width: parent.width * model.progress / 100
        color: Theme.highlightColor
        visible: model.status === DownloadModel.Running
    }
}
