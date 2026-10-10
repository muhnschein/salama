// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// View from browsing page: only it imports Sailfish.WebView (SCOPE.md §5). Engine renders
// all views into one picture, so sheet swaps in still of pressed page first.
//
// Landscape: band has no room for frame, so frame floats centred in room above sheet (where a
// pressed picture is lifted to), page shaped; toggle becomes a cell at band's right end.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Column {
    id: preview

    // Page link and front page not playing: hiding it would pause media.
    property bool offered: false
    property bool shown: false
    property Component pageView
    property real frameHeight: 0
    // Landscape only: room above sheet frame floats in, cell slot toggle fills, page's
    // width / height.
    property Item room: null
    property Item slot: null
    property real aspect: 1
    readonly property bool floating: room !== null
    // Floating frame's top, in room's parent's coordinates.
    readonly property real floatingTop: floating ? room.y + frame.y : 0

    signal openRequested()

    objectName: "linkPreview"
    visible: offered && !floating

    BackgroundItem {
        objectName: "linkPreviewToggle"
        width: parent.width
        height: Theme.itemSizeExtraSmall
        onClicked: Settings.linkPreview = !Settings.linkPreview

        Label {
            id: toggleLabel

            objectName: "linkPreviewToggleLabel"
            anchors {
                left: parent.left
                leftMargin: Theme.horizontalPageMargin
                right: chevron.left
                verticalCenter: parent.verticalCenter
            }
            //: The row over a link's preview, which hides it for every link
            text: Settings.linkPreview ? qsTr("Hide preview")
                                       //: The row a link's preview would be under, which shows it for every link
                                       : qsTr("Show preview")
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: parent.highlighted ? Theme.highlightColor : Theme.secondaryColor
        }

        Icon {
            id: chevron

            objectName: "linkPreviewChevron"
            anchors {
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
                verticalCenter: parent.verticalCenter
            }
            width: Theme.iconSizeExtraSmall
            height: width
            sourceSize: Qt.size(width, height)
            source: "image://theme/icon-m-right"
            rotation: Settings.linkPreview ? -90 : 90
            color: Theme.secondaryColor
        }
    }

    Item {
        id: frameSlot

        objectName: "linkPreviewFrame"
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * Theme.horizontalPageMargin
        height: Settings.linkPreview && !preview.floating ? preview.frameHeight
                                                            + Theme.paddingMedium : 0
        visible: Settings.linkPreview && !preview.floating
    }

    // Lies in its slot in the sheet, or floats in room above it: as large as fits, margins
    // round it.
    Rectangle {
        id: frame

        objectName: "linkPreviewPage"
        parent: preview.floating ? preview.room : frameSlot
        x: preview.floating ? (parent.width - width) / 2 : 0
        y: preview.floating ? (parent.height - height) / 2 : 0
        width: preview.floating ? Math.min(parent.width - 2 * Theme.horizontalPageMargin,
                                           (parent.height - 2 * Theme.paddingLarge)
                                           * preview.aspect)
                                : parent.width
        height: preview.floating ? width / preview.aspect : preview.frameHeight
        visible: preview.offered && Settings.linkPreview
        color: "transparent"
        border.width: Theme._lineWidth
        border.color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)

        Loader {
            objectName: "linkPreviewLoader"
            anchors {
                fill: parent
                margins: Theme._lineWidth
            }
            active: preview.shown
            sourceComponent: preview.pageView
        }

        // View only: swallow all touches.
        MouseArea {
            objectName: "linkPreviewTap"
            anchors.fill: parent
            onClicked: preview.openRequested()
        }
    }

    // Eye as Silica's password field draws it: open shows, struck through hides.
    MenuButton {
        objectName: "linkPreviewCell"
        parent: preview.slot
        width: parent ? parent.width : 0
        visible: preview.offered && preview.slot !== null
        labelSize: Theme.fontSizeExtraSmall
        iconSource: Settings.linkPreview ? "image://theme/icon-splus-hide-password"
                                         : "image://theme/icon-splus-show-password"
        text: toggleLabel.text
        onClicked: Settings.linkPreview = !Settings.linkPreview
    }
}
