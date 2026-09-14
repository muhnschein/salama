// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// One cell of the tab grid: the captured page preview with a close button in its
// top-right corner, and the favicon and title underneath.
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: preview

    signal closeRequested()

    objectName: "tabPreview"
    width: GridView.view.cellWidth
    height: GridView.view.cellHeight
    highlighted: down || model.activeTab

    Rectangle {
        id: shot

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: Theme.paddingMedium
        }
        height: parent.height - caption.height - Theme.paddingMedium * 3
        color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
        border.width: preview.highlighted ? Theme.paddingSmall / 2 : 0
        border.color: Theme.highlightColor

        Image {
            objectName: "tabPreviewImage"
            anchors.fill: parent
            fillMode: Image.PreserveAspectCrop
            clip: true
            asynchronous: true
            source: model.thumbnail.length > 0 ? "file://" + model.thumbnail : ""
            visible: status === Image.Ready
        }

        // Shown until the tab has been displayed at least once, and for private tabs,
        // whose pages are never written to disk.
        Label {
            objectName: "tabPreviewPlaceholder"
            anchors.centerIn: parent
            visible: model.thumbnail.length === 0
            text: model.privateTab ? qsTr("Private tab") : qsTr("No preview")
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
        }

        IconButton {
            objectName: "closeTabButton"
            anchors {
                right: parent.right
                top: parent.top
            }
            width: Theme.iconSizeMedium
            height: width
            icon.source: "image://theme/icon-m-clear"
            onClicked: preview.closeRequested()
        }
    }

    Row {
        id: caption

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: Theme.paddingMedium
        }
        height: Theme.iconSizeSmall
        spacing: Theme.paddingSmall

        Image {
            objectName: "tabFavicon"
            width: Theme.iconSizeSmall
            height: width
            fillMode: Image.PreserveAspectFit
            source: model.favicon
        }

        Label {
            objectName: "tabTitle"
            width: parent.width - Theme.iconSizeSmall - Theme.paddingSmall
            anchors.verticalCenter: parent.verticalCenter
            text: model.title.length > 0 ? model.title : model.url
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: preview.highlighted ? Theme.highlightColor : Theme.primaryColor
        }
    }
}
