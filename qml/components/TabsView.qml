// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// The tab grid. It is not a page: it sits directly below the browsing page, and the
// two are dragged over each other like a pulley. Pushing it onto the page stack
// instead gave the grid a sideways transition, which says "another screen" when what
// is meant is "the same screen, further down".
//
// The way back is this view's own overscroll. Dragged past its top it reports the
// distance and moves itself up by the same amount, which cancels the shift the
// flickable would otherwise draw: the content then stays exactly where the finger
// put it while the deck behind it slides down and brings the page back.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.tuuli 1.0

Item {
    id: tabsView

    // A finger has started, moved and released the pull that brings the page back.
    // The distance is how far the grid has been dragged past its own top.
    signal pullStarted()
    signal pulled(real distance)
    signal pullFinished(real distance)
    // A tab was chosen; the page is wanted back regardless of any drag.
    signal tabActivated()

    objectName: "tabsView"

    SilicaGridView {
        id: tabGrid

        // How far the view is dragged past its top, and the last such distance while
        // a finger was on it. The signal follows the finger only: the flickable's own
        // bounce back afterwards would otherwise fight the page's animation.
        readonly property real overscroll: Math.max(0, originY - contentY)
        property real pullDistance: 0

        objectName: "tabGrid"
        width: parent.width
        height: parent.height
        y: -overscroll
        model: TabModel
        cellWidth: width / 2
        cellHeight: cellWidth + Theme.itemSizeSmall
        // Vertical rather than automatic: with a handful of tabs the content fits the
        // screen, and an automatic flickable refuses to be dragged at all -- which is
        // exactly the case where the way back would go missing.
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.DragOverBounds

        onOverscrollChanged: {
            if (dragging) {
                pullDistance = overscroll
                tabsView.pulled(overscroll)
            }
        }
        onDragStarted: {
            pullDistance = 0
            tabsView.pullStarted()
        }
        onDragEnded: tabsView.pullFinished(pullDistance)

        // Room at the foot for the row below, which is drawn over the cells rather
        // than scrolling among them.
        footer: Item {
            width: tabGrid.width
            height: newTabRow.height
        }

        delegate: TabPreview {
            onTapped: {
                TabModel.activateTab(index)
                tabsView.tabActivated()
            }
            onCloseRequested: TabModel.closeTab(index)
            onMoveRequested: TabModel.moveTab(from, to)
        }

        ViewPlaceholder {
            enabled: TabModel.count === 0
            text: qsTr("No open tabs")
            hintText: qsTr("Open one with the button below")
        }

        VerticalScrollDecorator {}
    }

    // The one control the grid carries of its own, over the cells rather than among
    // them: a row along the foot of the view, in the same glass as the navigation bar.
    Rectangle {
        id: newTabRow

        objectName: "newTabRow"
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: Theme.itemSizeLarge
        color: Theme.rgba(Theme.highlightDimmerColor, Theme.opacityOverlay)

        IconButton {
            objectName: "newTabButton"
            anchors.centerIn: parent
            width: Theme.iconSizeMedium
            height: width
            icon.source: "image://theme/icon-m-add"
            onClicked: {
                TabModel.newTab(Settings.homePage)
                tabsView.tabActivated()
            }
        }
    }

    // The top edge is a pulley: dragged down it hands the page back.
    PullIndicator {
        objectName: "gridPullIndicator"
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: Theme.paddingSmall
        }
    }
}
