// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One of the start page's tiles: the site's icon on a rounded square and under it its
// name -- a bookmark's title, or the host as the bar shows it
// (docs/DECISIONS/0032-start-page.md). A site whose icon is not known, or does not
// load, has the first letter of its host on the square instead, as Firefox draws one.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

BackgroundItem {
    id: tile

    property string url
    property string title
    property string favicon
    readonly property string host: Settings.displayAddress(url)

    objectName: "siteTile"
    height: Theme.paddingMedium + square.height + Theme.paddingSmall + siteName.height
            + Theme.paddingMedium

    Rectangle {
        id: square

        objectName: "siteTileSquare"
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: Theme.paddingMedium
        }
        width: Theme.itemSizeMedium
        height: width
        radius: Theme.paddingMedium
        color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)

        Image {
            id: siteIcon

            objectName: "siteTileIcon"
            anchors.centerIn: parent
            width: Theme.iconSizeSmallPlus
            height: width
            fillMode: Image.PreserveAspectFit
            smooth: true
            asynchronous: true
            source: tile.favicon
            visible: status === Image.Ready
        }

        Label {
            objectName: "siteTileLetter"
            anchors.centerIn: parent
            visible: !siteIcon.visible
            text: tile.host.charAt(0).toUpperCase()
            font.pixelSize: Theme.fontSizeLarge
            color: tile.highlighted ? Theme.highlightColor : Theme.primaryColor
        }
    }

    Label {
        id: siteName

        objectName: "siteTileName"
        anchors {
            top: square.bottom
            topMargin: Theme.paddingSmall
            left: parent.left
            right: parent.right
            leftMargin: Theme.paddingSmall
            rightMargin: Theme.paddingSmall
        }
        horizontalAlignment: Text.AlignHCenter
        text: tile.title.length > 0 ? tile.title : tile.host
        truncationMode: TruncationMode.Fade
        font.pixelSize: Theme.fontSizeExtraSmall
        color: tile.highlighted ? Theme.highlightColor : Theme.primaryColor
    }
}
