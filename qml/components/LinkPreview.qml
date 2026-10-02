// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The link sheet's preview of the page a link leads to, between its head and its actions,
// as Safari's link preview is (docs/DECISIONS/0046-link-menu.md). A row that says Hide
// preview while it is shown and Show preview while it is not, and saying it is
// Settings.linkPreview: the preview opens, or stays shut, for every link from then on.
// Under the row, while it is open, the page in a frame of its own; a tap on it opens the
// link where the page was, as a tap on the link would have.
//
// The page is the browsing page's view, which that page alone can make: Sailfish.WebView
// is imported there and nowhere else (SCOPE.md §5). The engine draws every view it has
// into one picture, so the page the link was pressed on cannot be drawn while this one is;
// the sheet puts a still of it in its place first, and says shown once it has.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Column {
    id: preview

    // The link opens a page, and the page in front is not playing: hiding it to show
    // this would pause what it plays.
    property bool offered: false
    // The page the link leads to is in the frame.
    property bool shown: false
    // The browsing page's view for it.
    property Component pageView
    property real frameHeight: 0

    // The frame was tapped.
    signal openRequested()

    objectName: "linkPreview"
    visible: offered

    BackgroundItem {
        objectName: "linkPreviewToggle"
        width: parent.width
        height: Theme.itemSizeExtraSmall
        onClicked: Settings.linkPreview = !Settings.linkPreview

        Label {
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

        // A list row's chevron, turned to point where the frame goes.
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
        objectName: "linkPreviewFrame"
        x: Theme.horizontalPageMargin
        width: parent.width - 2 * Theme.horizontalPageMargin
        height: Settings.linkPreview ? preview.frameHeight + Theme.paddingMedium : 0
        visible: Settings.linkPreview

        Rectangle {
            id: frame

            width: parent.width
            height: preview.frameHeight
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

            // The page is to look at, not to use: every touch on it is this one's.
            MouseArea {
                objectName: "linkPreviewTap"
                anchors.fill: parent
                onClicked: preview.openRequested()
            }
        }
    }
}
