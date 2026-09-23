// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The tab grid. It is not a page: it sits directly below the browsing page, and the
// two are dragged over each other like a pulley. Pushing it onto the page stack
// instead gave the grid a sideways transition, which says "another screen" when what
// is meant is "the same screen, further down".
//
// The way back is this view's own overscroll. Dragged past its top it reports the
// distance and moves itself up by the same amount, which cancels the shift the
// flickable would otherwise draw: the content then stays exactly where the finger
// put it while the deck behind it slides down and brings the page back. It is
// dragged from anywhere on the screen: the cells leave a drag up or down to it, and
// the two rows drawn over the cells are the flickable's own children, so it sees
// every press on them too -- though a pull begun on the foot row has too little
// screen below it to bring the page back.
//
// The head row holds the way to a new tab and the search for one; the foot row holds
// the groups, within reach of the thumb that carries a cell down to one of them to
// change its group (docs/DECISIONS/0015-tab-groups.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

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

    // The rows are declared beside the grid, where the view's own default property
    // cannot take them into the content it scrolls, and are handed to the grid once
    // made. A flickable filters the presses of its own children and nothing else's:
    // beside it, the group strip took every press on the head of the screen, and
    // the page could not be pulled back from there.
    Component.onCompleted: {
        headRow.parent = tabGrid
        footRow.parent = tabGrid
    }

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
            height: footRow.height
        }

        // By id rather than by row: the grid's rows are the current group's, and
        // the tab model's are every group's. A cell carried down onto the strip of
        // groups is put into the group it is dropped on, and whatever the strip lit
        // goes out when the carry ends, dropped or cancelled.
        delegate: TabPreview {
            dropTarget: groupStrip
            onHeldChanged: {
                if (!held) {
                    groupStrip.endCarry()
                }
            }
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
            hintText: qsTr("Open one with the button above")
        }

        VerticalScrollDecorator {}
    }

    // The way to a new tab in one corner and the search for a tab in the other, over
    // the cells rather than among them. The row is also what keeps the top row of
    // cells clear of the screen's cutout. It rides on the grid, which moves up as it
    // is pulled down; the margin keeps the row where the content is.
    Rectangle {
        id: headRow

        objectName: "gridHeadRow"
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: tabGrid.overscroll
        }
        // The cutout on top of the row's own height, and the controls below the
        // cutout rather than centred through it: the row starts at the top of the
        // screen, and the notch was taking a bite out of what it carries.
        height: Theme.itemSizeLarge + tabsView.cutoutHeight
        color: Theme.rgba(Theme.highlightDimmerColor, Theme.opacityOverlay)

        // The grid's own edge is a pulley too: dragged down it hands the page back.
        // Said the way Silica says a pulley menu is there -- a line across the whole
        // edge -- rather than with the bar's handle, which on device read as a second
        // handle to find. The highlight background colour: the highlight itself was
        // too loud a line to have across the top of every grid.
        Rectangle {
            objectName: "gridPullIndicator"
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }
            height: Theme.paddingSmall
            color: Theme.highlightBackgroundColor
        }

        Item {
            objectName: "gridHeadControls"
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            height: Theme.itemSizeLarge

            // Held rather than tapped, the button brings up what was closed lately.
            IconButton {
                objectName: "newTabButton"
                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                width: Theme.iconSizeMedium
                height: width
                icon.source: "image://theme/icon-m-add"
                onClicked: {
                    TabModel.newTab(Settings.homePage)
                    tabsView.tabActivated()
                }
                onPressAndHold: closedPanel.show()
            }

            // A page of its own over the grid, which stays open under it for when it
            // is popped.
            IconButton {
                objectName: "searchTabsButton"
                anchors {
                    right: parent.right
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                width: Theme.iconSizeMedium
                height: width
                icon.source: "image://theme/icon-m-search"
                onClicked: pageStack.push(Qt.resolvedUrl("../pages/TabSearchPage.qml"))
            }
        }
    }

    // The groups, along the foot of the view in the same glass as the navigation bar,
    // with the way to edit them in the corner. On the grid as the head row is, and
    // held still against its pull the same way.
    Rectangle {
        id: footRow

        objectName: "gridFootRow"
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            bottomMargin: -tabGrid.overscroll
        }
        height: Theme.itemSizeLarge
        color: Theme.rgba(Theme.highlightDimmerColor, Theme.opacityOverlay)

        TabGroupStrip {
            id: groupStrip

            anchors.fill: parent
            // A page of its own over the grid, which stays open under it for when it
            // is popped.
            onEditRequested: pageStack.push(Qt.resolvedUrl("../pages/TabGroupsPage.qml"))
        }
    }

    RecentlyClosedPanel {
        id: closedPanel

        width: parent.width
        height: Math.round(parent.height * 0.6)
        onTabReopened: tabsView.tabActivated()
    }
}
