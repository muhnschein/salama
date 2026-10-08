// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Link texts and addresses are page-controlled: plain text only.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: header

    // EngineMessages.linkTarget() map.
    property var target
    property string appIcon

    readonly property bool hasTitle: target.title.length > 0

    signal copyRequested()

    objectName: "linkMenuHeader"
    height: Theme.itemSizeSmall

    Rectangle {
        id: tile

        objectName: "linkMenuTile"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmallPlus
        height: width
        radius: Theme.paddingSmall
        color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
        clip: true

        Image {
            id: thumbnail

            objectName: "linkMenuThumbnail"
            anchors.fill: parent
            source: header.target.image
            sourceSize.width: width
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            visible: status === Image.Ready
        }

        Icon {
            objectName: "linkMenuAppIcon"
            anchors.centerIn: parent
            visible: header.appIcon.length > 0 && !thumbnail.visible
            source: header.appIcon
            sourceSize: Qt.size(Theme.iconSizeSmall, Theme.iconSizeSmall)
        }

        Label {
            objectName: "linkMenuInitial"
            anchors.centerIn: parent
            visible: header.appIcon.length === 0 && !thumbnail.visible
            text: header.target.address.charAt(0).toUpperCase()
            textFormat: Text.PlainText
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryColor
        }
    }

    Column {
        anchors {
            left: tile.right
            leftMargin: Theme.paddingMedium
            right: copyButton.left
            verticalCenter: parent.verticalCenter
        }

        // Textless link (image, bare url): named by target, shown once.
        Label {
            objectName: "linkMenuTitle"
            width: parent.width
            text: header.hasTitle ? header.target.title : header.target.address
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeSmall
        }

        Label {
            objectName: "linkMenuAddress"
            width: parent.width
            visible: header.hasTitle
            text: header.target.address
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
        }
    }

    IconButton {
        id: copyButton

        objectName: "copyLinkButton"
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin - Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmallPlus + 2 * Theme.paddingMedium
        height: parent.height
        icon.source: "image://theme/icon-m-clipboard"
        icon.sourceSize: Qt.size(Theme.iconSizeSmallPlus, Theme.iconSizeSmallPlus)
        onClicked: header.copyRequested()
    }
}
