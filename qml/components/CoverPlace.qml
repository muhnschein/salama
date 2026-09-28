// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The cover at rest: where the reader was. The site of the tab in front, its icon beside
// it, and the page's title, large, as much of it as there is room for
// (docs/DECISIONS/0037-cover-is-where-you-were.md).
//
// Words only, over the cover's faint halftone: nothing here is a picture of the page. It
// changes as the tab in front does, or its page, and is still in between.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: place

    /// The page in front: its address, title and icon.
    property string url
    property string title
    property string favicon

    /// The site, as the bar shows it.
    readonly property string host: SearchSettings.displayAddress(url)

    objectName: "coverPlace"

    // The site's icon on a faint square, or, while it has none, the first letter of its
    // host, as the start page's tiles draw one.
    Rectangle {
        id: tile

        objectName: "coverPlaceTile"
        anchors.verticalCenter: hostLabel.verticalCenter
        width: hostLabel.height
        height: width
        radius: Theme.paddingSmall
        color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)

        Image {
            id: siteIcon

            objectName: "coverPlaceIcon"
            anchors {
                fill: parent
                margins: Theme.paddingSmall / 2
            }
            fillMode: Image.PreserveAspectFit
            smooth: true
            asynchronous: true
            source: place.favicon
            visible: status === Image.Ready
        }

        Label {
            objectName: "coverPlaceLetter"
            anchors.centerIn: parent
            visible: !siteIcon.visible
            textFormat: Text.PlainText
            text: place.host.charAt(0).toUpperCase()
            font.pixelSize: Theme.fontSizeTiny
            color: Theme.primaryColor
        }
    }

    Label {
        id: hostLabel

        objectName: "coverPlaceHost"
        anchors {
            top: parent.top
            left: tile.right
            right: parent.right
            leftMargin: Theme.paddingSmall
        }
        textFormat: Text.PlainText
        text: place.host
        truncationMode: TruncationMode.Fade
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.highlightColor
    }

    // As many lines as fit under the site, the last cut short.
    Label {
        objectName: "coverPlaceTitle"
        anchors {
            top: hostLabel.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            topMargin: Theme.paddingSmall
        }
        textFormat: Text.PlainText
        text: place.title.length > 0 ? place.title : place.host
        wrapMode: Text.Wrap
        elide: Text.ElideRight
        font.pixelSize: Theme.fontSizeMedium
        color: Theme.primaryColor
    }
}
