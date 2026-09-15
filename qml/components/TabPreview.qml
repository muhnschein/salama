// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// One cell of the tab grid: the captured page preview with a close button in its
// top-right corner, and the favicon and title underneath.
//
// The cell can also be carried to another place in the grid. One MouseArea under the
// contents owns the press, the way the navigation bar's does, so the tap, the carry
// and the close button do not fight over it: the button is drawn above the handler
// and keeps its own taps, everything else falls through to it.
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: preview

    // Its own tap signal rather than BackgroundItem's clicked: that one comes from
    // MouseArea and carries a mouse event, which this handler has not got to give it.
    signal tapped()
    signal closeRequested()
    // The cell has been carried over another one and the two should trade places.
    signal moveRequested(int from, int to)

    // True while this cell is being carried rather than merely pressed.
    property bool held: false
    readonly property Item grid: GridView.view

    objectName: "tabPreview"
    width: GridView.view.cellWidth
    height: GridView.view.cellHeight
    highlighted: dragArea.pressed || model.activeTab
    // A carried cell passes over its neighbours, not under them.
    z: held ? 1 : 0

    function drop() {
        held = false
        content.x = 0
        content.y = 0
    }

    MouseArea {
        id: dragArea

        property real grabX: 0
        property real grabY: 0

        objectName: "tabPreviewGesture"
        anchors.fill: parent
        // Once the cell is being carried the grid may not take the drag back.
        preventStealing: preview.held

        onPressed: {
            grabX = mouse.x
            grabY = mouse.y
        }
        onPositionChanged: {
            if (!preview.held) {
                var acrossX = mouse.x - grabX
                var acrossY = mouse.y - grabY
                // Sideways, because the grid itself only flicks up and down: a drag
                // across the cell is the one movement nothing else is waiting for.
                if (Math.abs(acrossX) > Theme.startDragDistance
                        && Math.abs(acrossX) > Math.abs(acrossY)) {
                    preview.held = true
                }
            }
            if (preview.held) {
                // The contents move, not the cell: the view owns where cells are, and
                // after a trade the cell underneath has already moved to meet them.
                content.x = mouse.x - grabX
                content.y = mouse.y - grabY
                var target = preview.grid.indexAt(preview.x + mouse.x, preview.y + mouse.y)
                if (target >= 0 && target !== index) {
                    preview.moveRequested(index, target)
                }
            }
        }
        onReleased: preview.drop()
        onCanceled: preview.drop()
        onClicked: {
            if (!preview.held) {
                preview.tapped()
            }
        }
    }

    Item {
        id: content

        width: parent.width
        height: parent.height

        Rectangle {
            id: shot

            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                margins: Theme.paddingMedium
            }
            height: parent.height - caption.height - Theme.paddingMedium * 3
            color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
            border.width: preview.highlighted ? Theme.paddingSmall / 2 : 0
            border.color: Theme.highlightColor

            Image {
                objectName: "tabPreviewImage"
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                clip: true
                asynchronous: true
                source: model.thumbnail.length > 0 ? "file://" + model.thumbnail : ""
                visible: status === Image.Ready
            }

            // Shown until the tab has been displayed at least once, and for private
            // tabs, whose pages are never written to disk.
            Label {
                objectName: "tabPreviewPlaceholder"
                anchors.centerIn: parent
                visible: model.thumbnail.length === 0
                text: model.privateTab ? qsTr("Private tab") : qsTr("No preview")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }

            IconButton {
                objectName: "closeTabButton"
                anchors {
                    right: parent.right
                    top: parent.top
                }
                width: Theme.iconSizeMedium
                height: width
                icon.source: "image://theme/icon-m-clear"
                onClicked: preview.closeRequested()
            }
        }

        Row {
            id: caption

            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                margins: Theme.paddingMedium
            }
            height: Theme.iconSizeSmall
            spacing: Theme.paddingSmall

            Image {
                objectName: "tabFavicon"
                width: Theme.iconSizeSmall
                height: width
                fillMode: Image.PreserveAspectFit
                source: model.favicon
            }

            Label {
                objectName: "tabTitle"
                width: parent.width - Theme.iconSizeSmall - Theme.paddingSmall
                anchors.verticalCenter: parent.verticalCenter
                text: model.title.length > 0 ? model.title : model.url
                truncationMode: TruncationMode.Fade
                font.pixelSize: Theme.fontSizeExtraSmall
                color: preview.highlighted ? Theme.highlightColor : Theme.primaryColor
            }
        }
    }
}
