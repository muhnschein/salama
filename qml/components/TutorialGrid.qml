// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Real TabPreview cells over sketched ListModel, so gestures match real grid.
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
            height: headRow.height
        }

        footer: Item {
            width: view.width
            height: footRow.height
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
