// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Real TabPreview cells over sketched ListModel, so gestures match real grid. Landscape, as
// tab grid: no head or foot row; groups in rail left of grid.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: tutorialGrid

    signal pullStarted()
    signal pulled(real distance)
    signal pullFinished(real distance)
    signal tabTapped()
    signal tabClosed()
    signal tabMoved()
    signal tabGrouped()

    property real cutoutHeight: 0
    readonly property bool wide: width > height
    readonly property real railWidth: wide ? Math.round(width / 5) : 0
    readonly property GridCells cells: GridCells {
        width: tutorialGrid.width - tutorialGrid.railWidth
        height: tutorialGrid.height
        pageWidth: tutorialGrid.width
    }
    readonly property int count: tabs.count
    readonly property Item strip: groupStrip
    property bool moved: false
    property bool handling: false

    function reset() {
        var pages = ["news", "shop", "article", "dark"]
        tabs.clear()
        for (var i = 0; i < pages.length; ++i) {
            // TabPreview prefixes file://.
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

    function cellCentre(index) {
        return Qt.point(railWidth + (index % cells.columns + 0.5) * view.cellWidth,
                        (wide ? 0 : headRow.height)
                        + (Math.floor(index / cells.columns) + 0.5) * view.cellHeight)
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

    // Under grid, so carried cell shows over group it is dropped on.
    Rectangle {
        objectName: "tutorialRail"
        width: tutorialGrid.railWidth
        height: parent.height
        visible: tutorialGrid.wide
        color: Theme.highlightDimmerColor

        GlassTexture {
            anchors.fill: parent
        }

        // Search field sketch.
        Item {
            id: railHead

            width: parent.width
            height: Theme.itemSizeSmall

            Icon {
                id: searchIcon

                anchors {
                    left: parent.left
                    leftMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                width: Theme.iconSizeSmallPlus
                height: width
                source: "image://theme/icon-m-search"
            }

            Rectangle {
                anchors {
                    left: searchIcon.right
                    right: parent.right
                    leftMargin: Theme.paddingMedium
                    rightMargin: Theme.horizontalPageMargin
                    verticalCenter: parent.verticalCenter
                }
                height: Theme.paddingMedium
                radius: height / 2
                color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
            }
        }

        Item {
            id: railGroups

            anchors {
                left: parent.left
                right: parent.right
                top: railHead.bottom
                bottom: parent.bottom
            }
        }
    }

    SilicaGridView {
        id: view

        readonly property real overscroll: Math.max(0, originY - contentY)
        property real pullDistance: 0

        objectName: "tutorialGridView"
        x: tutorialGrid.railWidth
        width: parent.width - x
        height: parent.height
        y: -overscroll
        model: tabs
        cellWidth: cells.cellWidth
        cellHeight: cells.cellHeight
        // Explicit vertical: automatic refuses drag when content fits.
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
            height: tutorialGrid.wide ? 0 : headRow.height
        }

        footer: Item {
            width: view.width
            height: tutorialGrid.wide ? 0 : footRow.height
        }

        delegate: TabPreview {
            dropTarget: groupStrip
            // Carry with swap counts as move once finger lifts.
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
        visible: !tutorialGrid.wide
        color: Theme.highlightDimmerColor

        GlassTexture {
            anchors.fill: parent
        }

        // Landscape: along whole top, over rail too.
        Rectangle {
            objectName: "tutorialPullIndicator"
            parent: tutorialGrid.wide ? tutorialGrid : headRow
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
        visible: !tutorialGrid.wide
        color: Theme.highlightDimmerColor

        GlassTexture {
            anchors.fill: parent
        }

        TutorialStrip {
            id: groupStrip

            objectName: "tutorialStrip"
            parent: tutorialGrid.wide ? railGroups : footRow
            anchors.fill: parent
            vertical: tutorialGrid.wide
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
