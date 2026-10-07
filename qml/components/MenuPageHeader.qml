// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Menu sheet head: page icon, title, host, copy-address button; tap opens site details.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: header

    property string url
    property string title
    property string favicon
    property bool tlsBroken: false
    readonly property bool hasPage: url.length > 0
    readonly property string host: hasPage ? SearchSettings.displayAddress(url) : ""
    // Empty band above and below drawn content; sheet sizes gaps by ink, not item.
    readonly property real inkMargin: (height - Math.max(tile.height, lines.height)) / 2

    signal copyRequested()
    signal detailsRequested()

    objectName: "menuHeader"
    height: Theme.itemSizeSmall

    // Copy button on top takes own taps.
    BackgroundItem {
        objectName: "menuHeaderDetails"
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
            right: copyButton.visible ? copyButton.left : parent.right
        }
        enabled: header.hasPage
        onClicked: header.detailsRequested()
    }

    // Tile so favicon drawn for light page still has ground.
    Rectangle {
        id: tile

        objectName: "menuPageTile"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmallPlus
        height: width
        radius: Theme.paddingSmall
        color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)

        Image {
            id: siteIcon

            objectName: "menuPageFavicon"
            anchors.centerIn: parent
            width: Theme.iconSizeSmall
            height: width
            fillMode: Image.PreserveAspectFit
            source: header.hasPage ? header.favicon : ""
            visible: status === Image.Ready
        }

        Label {
            objectName: "menuPageInitial"
            anchors.centerIn: parent
            visible: header.hasPage && !siteIcon.visible
            text: header.host.charAt(0).toUpperCase()
            textFormat: Text.PlainText
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryColor
        }

        Icon {
            objectName: "menuStartPageIcon"
            anchors.centerIn: parent
            visible: !header.hasPage
            source: "image://theme/icon-m-home"
            sourceSize: Qt.size(Theme.iconSizeSmall, Theme.iconSizeSmall)
        }
    }

    Column {
        id: lines

        anchors {
            left: tile.right
            leftMargin: Theme.paddingMedium
            right: chevron.visible ? chevron.left : parent.right
            rightMargin: chevron.visible ? Theme.paddingSmall : Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        Label {
            objectName: "menuPageTitle"
            width: parent.width
            //: The head of the browser's menu on the start page, where there is no page
            text: !header.hasPage ? qsTr("Start page")
                                  : header.title.length > 0 ? header.title : header.host
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeSmall
        }

        Item {
            width: parent.width
            height: hostLabel.height
            visible: header.hasPage

            Icon {
                id: securityIcon

                objectName: "menuPageSecurity"
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.iconSizeExtraSmall
                height: width
                sourceSize: Qt.size(width, height)
                visible: header.url.indexOf("https://") === 0
                source: header.tlsBroken ? "image://theme/icon-s-filled-warning"
                                         : "image://theme/icon-s-outline-secure"
                color: header.tlsBroken ? Theme.errorColor : Theme.secondaryColor
            }

            Label {
                id: hostLabel

                objectName: "menuPageHost"
                x: securityIcon.visible ? securityIcon.width + Theme.paddingSmall : 0
                width: parent.width - x
                text: header.host
                textFormat: Text.PlainText
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }
        }
    }

    Icon {
        id: chevron

        objectName: "menuHeaderChevron"
        anchors {
            right: copyButton.left
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeExtraSmall
        height: width
        sourceSize: Qt.size(width, height)
        visible: header.hasPage
        source: "image://theme/icon-m-right"
        color: Theme.secondaryColor
    }

    IconButton {
        id: copyButton

        objectName: "copyAddressButton"
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin - Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeSmallPlus + 2 * Theme.paddingMedium
        height: parent.height
        visible: header.hasPage
        icon.source: "image://theme/icon-m-clipboard"
        icon.sourceSize: Qt.size(Theme.iconSizeSmallPlus, Theme.iconSizeSmallPlus)
        onClicked: header.copyRequested()
    }
}
