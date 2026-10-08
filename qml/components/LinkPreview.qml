// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// View from browsing page: only it imports Sailfish.WebView (SCOPE.md §5). Engine renders
// all views into one picture, so sheet swaps in still of pressed page first.
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

            // View only: swallow all touches.
            MouseArea {
                objectName: "linkPreviewTap"
                anchors.fill: parent
                onClicked: preview.openRequested()
            }
        }
    }
}
