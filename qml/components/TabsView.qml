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

    // What the display's cutout takes at the top of the screen, as the page works it
    // out and the settings allow. Silica's own PullDownMenu adds the same to its top
    // margin in portrait (docs/DECISIONS/0013-screen-cutout.md).
    property real cutoutHeight: 0

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
        model: GroupTabs
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

        // Room at the head and the foot for the two rows below, which are drawn over
        // the cells rather than scrolling among them. The head also keeps the first
        // row of cells -- and the close button in its corner -- out from under the
        // device's own cutout.
        header: Item {
            width: tabGrid.width
            height: headRow.height
        }

        footer: Item {
            width: tabGrid.width
            height: newTabRow.height
        }

        // By id rather than by row: the grid's rows are the current group's, and
        // the tab model's are every group's.
        delegate: TabPreview {
            onTapped: {
                TabModel.activateTabById(model.tabId)
                tabsView.tabActivated()
            }
            onCloseRequested: TabModel.closeTabById(model.tabId)
            onMoveRequested: GroupTabs.moveTab(from, to)
        }

        ViewPlaceholder {
            enabled: GroupTabs.count === 0
            text: qsTr("No tabs in this group")
            hintText: qsTr("Open one with the button below")
        }

        VerticalScrollDecorator {}
    }

    // The groups, over the cells rather than among them, with the way to edit them in
    // one corner and the search for a tab in the other. The row is also what keeps
    // the top row of cells clear of the screen's cutout.
    Rectangle {
        id: headRow

        objectName: "tabGroupRow"
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        // The cutout on top of the row's own height, and the strip below the cutout
        // rather than centred through it: the row starts at the top of the screen,
        // and the notch was taking a bite out of what it says.
        height: Theme.itemSizeLarge + tabsView.cutoutHeight
        color: Theme.rgba(Theme.highlightDimmerColor, Theme.opacityOverlay)

        // The grid's own edge is a pulley too: dragged down it hands the page back.
        // Said the way Silica says a pulley menu is there -- a line in the highlight
        // colour across the whole edge -- rather than with the bar's handle, which on
        // device read as a second handle to find.
        Rectangle {
            objectName: "gridPullIndicator"
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }
            height: Theme.paddingSmall
            color: Theme.highlightColor
        }

        TabGroupStrip {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            height: Theme.itemSizeLarge
            // Both are pages of their own over the grid, which stays open under them
            // for when they are popped.
            onEditRequested: pageStack.push(Qt.resolvedUrl("../pages/TabGroupsPage.qml"))
            onSearchRequested: pageStack.push(Qt.resolvedUrl("../pages/TabSearchPage.qml"))
        }
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

        // Held rather than tapped, the button brings up what was closed lately.
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
            onPressAndHold: closedPanel.show()
        }
    }

    RecentlyClosedPanel {
        id: closedPanel

        width: parent.width
        height: Math.round(parent.height * 0.6)
        onTabReopened: tabsView.tabActivated()
    }
}
