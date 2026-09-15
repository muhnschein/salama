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

    // True while this cell is being carried rather than merely pressed, and true from
    // the moment it is picked up until the next press. MouseArea raises released
    // before clicked, so the first cannot be what clears the second.
    property bool held: false
    property bool carried: false
    readonly property Item grid: GridView.view

    objectName: "tabPreview"
    width: GridView.view.cellWidth
    height: GridView.view.cellHeight
    highlighted: dragArea.pressed || model.activeTab
    // A carried cell passes over its neighbours, not under them.
    z: held ? 1 : 0

    // A cell that has been carried must not also open on release. MouseArea raises
    // released before clicked, so held is already false by then and cannot be the
    // guard; carried lives until the next press.
    function releaseTap() {
        if (!carried) {
            tapped()
        }
    }

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
            preview.carried = false
        }
        onPositionChanged: {
            if (!preview.held) {
                var acrossX = mouse.x - grabX
                var acrossY = mouse.y - grabY
                // Sideways, because the grid itself only flicks up and down: a drag
                // across the cell is the one movement nothing else is waiting for.
                // Half the usual drag distance, because nothing else is waiting for
                // it: the cell comes up almost as soon as the finger moves across.
                if (Math.abs(acrossX) > Theme.startDragDistance / 2
                        && Math.abs(acrossX) > Math.abs(acrossY)) {
                    preview.held = true
                    preview.carried = true
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
        onClicked: preview.releaseTap()
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
            clip: true
            color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
            border.width: preview.highlighted ? Theme.paddingSmall / 2 : 0
            border.color: Theme.highlightColor

            // As wide as the cell and anchored to its top, at the picture's own
            // aspect: what shows is then the top of what was last on the screen.
            // PreserveAspectCrop centres instead, and a screen-shaped picture in a
            // cell-shaped box centres on the middle of the page -- which is neither
            // where the reader was at the top of a page nor where they were at its
            // foot.
            Image {
                objectName: "tabPreviewImage"
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                }
                height: sourceSize.width > 0 ? width * sourceSize.height / sourceSize.width
                                             : parent.height
                fillMode: Image.PreserveAspectFit
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
