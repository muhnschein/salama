// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: place

    property string url
    property string title
    property string favicon

    readonly property string host: SearchSettings.displayAddress(url)

    objectName: "coverPlace"

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
