// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The head of the link sheet, naming what was pressed as the menu's head names the page
// (docs/DECISIONS/0046-link-menu.md, 0021-menu-sheet.md): on a faint tile the picture
// pressed, or the icon of the application another kind of link is for, or the host's
// initial; the link's text, and under it where it goes -- the host and the path, the
// mailbox, the number. At the right, the menu head's button to copy it.
//
// Link texts and addresses are what pages chose, and are drawn as plain text.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: header

    // What was pressed: EngineMessages.linkTarget()'s map.
    property var target
    // The theme icon of the application a link is for, when it is for one.
    property string appIcon

    readonly property bool hasTitle: target.title.length > 0

    // The copy button was tapped.
    signal copyRequested()

    objectName: "linkMenuHeader"
    height: Theme.itemSizeSmall

    // The address bar's suggestions' faint tile, so that a picture or a letter has a
    // ground under it whatever the ambience.
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

        // A link without text of its own -- a picture, or a bare address -- is named by
        // where it goes, once.
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

    // As the menu head's: at the page margin inside a button a padding wider either side,
    // the keyboard's paste key's clipboard (MenuPageHeader.qml).
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
