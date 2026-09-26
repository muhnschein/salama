// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The tab grid as the tutorial draws it: the glass rows at its head and foot, the line
// across the top that says the edge can be pulled, the strip of groups at the foot, and
// cells of made-up pages between (docs/DECISIONS/0034-tutorial.md). The cells are the
// real grid's, TabPreview, over a model of four sketched tabs rather than TabModel, so a
// tab is slid away, held and carried, or carried onto a group, by the same hand as in the
// real grid; what happens to it happens to the sketch alone.
//
// The way back to the page is what it is on the real grid: the view's own overscroll,
// reported as a distance, with the view moved up by as much so the cells stay under the
// finger while the deck behind them slides down (docs/DECISIONS/0010-tab-grid-deck.md).
// The rows are children of the view, as the real grid's are, so a pull begun on either is
// the view's.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: tutorialGrid

    signal pullStarted()
    signal pulled(real distance)
    signal pullFinished(real distance)
    // A cell was tapped, slid away or closed, carried to another place and released, or
    // dropped on the other group.
    signal tabTapped()
    signal tabClosed()
    signal tabMoved()
    signal tabGrouped()

    // What the display's cutout takes at the top of the screen, which the head row is
    // taller by, as the real grid's is.
    property real cutoutHeight: 0
    readonly property int count: tabs.count
    readonly property Item strip: groupStrip
    // The cell being carried has traded places with another.
    property bool moved: false
    // A cell is held or being slid: a finger is making a gesture on the grid.
    property bool handling: false

    // The sketch's four tabs again, the first in front.
    function reset() {
        var pages = ["news", "shop", "article", "dark"]
        tabs.clear()
        for (var i = 0; i < pages.length; ++i) {
            // A path, as TabModel's thumbnails are, which TabPreview makes a file url of.
            var picture = String(Qt.resolvedUrl("../../art/tutorial/" + pages[i] + ".png"))
            tabs.append({
                tabId: i + 1,
                thumbnail: decodeURIComponent(picture.replace(/^file:\/\//, "")),
                url: pages[i],
                activeTab: i === 0,
                mediaState: TabModel.NoMedia,
                muted: false
            })
        }
    }

    // The middle of a cell, in the grid's own coordinates.
    function cellCentre(index) {
        return Qt.point((index % 2 + 0.5) * view.cellWidth,
                        headRow.height + (Math.floor(index / 2) + 0.5) * view.cellHeight)
    }

    function rowOf(tabId) {
        for (var i = 0; i < tabs.count; ++i) {
            if (tabs.get(i).tabId === tabId) {
                return i
            }
        }
        return -1
    }

    Component.onCompleted: {
        reset()
        headRow.parent = view
        footRow.parent = view
    }

    ListModel {
        id: tabs
    }

    SilicaGridView {
        id: view

        readonly property real overscroll: Math.max(0, originY - contentY)
        property real pullDistance: 0

        objectName: "tutorialGridView"
        width: parent.width
        height: parent.height
        y: -overscroll
        model: tabs
        cellWidth: width / 2
        cellHeight: cellWidth + Theme.itemSizeSmall
        // Vertical rather than automatic: the cells fit the screen, and an automatic
        // flickable whose content fits refuses to be dragged at all.
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior: Flickable.DragOverBounds

        onOverscrollChanged: {
            if (dragging) {
                pullDistance = overscroll
                tutorialGrid.pulled(overscroll)
            }
        }
        onDragStarted: {
            pullDistance = 0
            tutorialGrid.pullStarted()
        }
        onDragEnded: tutorialGrid.pullFinished(pullDistance)

        header: Item {
            width: view.width
            height: headRow.height
        }

        footer: Item {
            width: view.width
            height: footRow.height
        }

        delegate: TabPreview {
            dropTarget: groupStrip
            // A carry that traded places with another cell has moved a tab once the
            // finger lifts.
            onSwipingChanged: tutorialGrid.handling = held || swiping
            onHeldChanged: {
                tutorialGrid.handling = held || swiping
                if (held) {
                    tutorialGrid.moved = false
                } else {
                    groupStrip.endCarry()
                    if (tutorialGrid.moved) {
                        tutorialGrid.tabMoved()
                    }
                }
            }
            onTapped: tutorialGrid.tabTapped()
            onCloseRequested: {
                tabs.remove(index)
                tutorialGrid.tabClosed()
            }
            onMoveRequested: {
                tabs.move(from, to, 1)
                tutorialGrid.moved = true
            }
        }
    }

    Rectangle {
        id: headRow

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: view.overscroll
        }
        height: Theme.itemSizeLarge + tutorialGrid.cutoutHeight
        color: Theme.highlightDimmerColor

        GlassTexture {
            anchors.fill: parent
        }

        Rectangle {
            objectName: "tutorialPullIndicator"
            width: parent.width
            height: Theme.paddingSmall
            color: Theme.highlightBackgroundColor
        }
    }

    Rectangle {
        id: footRow

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            bottomMargin: -view.overscroll
        }
        height: Theme.itemSizeLarge
        color: Theme.highlightDimmerColor

        GlassTexture {
            anchors.fill: parent
        }

        TutorialStrip {
            id: groupStrip

            objectName: "tutorialStrip"
            anchors.fill: parent
            tabCount: tabs.count
            onTabDropped: {
                var row = tutorialGrid.rowOf(tabId)
                if (row >= 0) {
                    tabs.remove(row)
                    tutorialGrid.tabGrouped()
                }
            }
        }
    }
}
