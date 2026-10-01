// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The head of the menu sheet, naming the page its actions are for: the page's icon, its
// title, and under the title a padlock for a page that came over https and its host, as
// the bar shows it (docs/DECISIONS/0021-menu-sheet.md). At the right, a button that puts
// the page's address on the clipboard, as sailfish-browser does from its toolbar
// (apps/browser/qml/pages/components/ToolBar.qml). A tap on the rest of the head opens
// the page's site details, which a chevron after the title says are there
// (docs/DECISIONS/0040-site-details.md). On the start page there is no page and no
// address: the head says so, beside the theme's home, and there is nothing to copy and
// no site to show the details of.
//
// Titles and hosts are what pages chose to be called, and are drawn as plain text.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: header

    // The page: its address, which is empty on the start page, its title and its icon.
    property string url
    property string title
    property string favicon
    // The engine is not satisfied with the connection, as the bar's warning says.
    property bool tlsBroken: false
    readonly property bool hasPage: url.length > 0
    readonly property string host: hasPage ? SearchSettings.displayAddress(url) : ""

    // The copy button was tapped.
    signal copyRequested()
    // The rest of the head was tapped: the page's site details are wanted.
    signal detailsRequested()

    objectName: "menuHeader"
    height: Theme.itemSizeSmall

    // The head as a whole is the way to the site details, lit while it is pressed as
    // Silica's rows are; the copy button, over it at the right, takes its own taps.
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

    // The page's icon on a tile of its own, the address bar's suggestions' faint tile,
    // so that an icon drawn for a light page still has a ground under it.
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

        // A page without an icon of its own: its host's initial, as the address bar's
        // suggestions draw one.
        Label {
            objectName: "menuPageInitial"
            anchors.centerIn: parent
            visible: header.hasPage && !siteIcon.visible
            text: header.host.charAt(0).toUpperCase()
            textFormat: Text.PlainText
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryColor
        }

        // The theme's home, as sailfish-browser draws beside its home page setting
        // (apps/browser/qml/pages/SettingsPage.qml).
        Icon {
            objectName: "menuStartPageIcon"
            anchors.centerIn: parent
            visible: !header.hasPage
            source: "image://theme/icon-m-home"
            sourceSize: Qt.size(Theme.iconSizeSmall, Theme.iconSizeSmall)
        }
    }

    Column {
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

            // The padlock sailfish-browser's toolbar draws for https, and while the engine
            // is unhappy with the connection the warning the bar draws, in its colour.
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

    // After the title, small: there is more behind it, as a list row's has.
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

    // At the page margin inside a button a padding wider either side and the row's
    // height, as the grid's corner buttons are. The theme's clipboard, as the keyboard's
    // paste key draws it (maliit com/jolla/PasteButton.qml).
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
