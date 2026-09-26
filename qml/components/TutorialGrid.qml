// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The tab grid as the tutorial draws it: the two rows of glass at its head and foot, the
// line across the top that says the edge can be pulled, and between them cells that
// stand for tabs, the first washed as the tab in front is (docs/DECISIONS/0034-tutorial.md).
// None of it is a tab, so nothing here opens or closes one.
//
// The way back to the page is what it is on the real grid: the view's own overscroll,
// reported as a distance, with the view moved up by as much so the cells stay under the
// finger while the deck behind them slides down (docs/DECISIONS/0010-tab-grid-deck.md).
// The rows take no presses, so a pull begun on either is the view's.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: tutorialGrid

    signal pullStarted()
    signal pulled(real distance)
    signal pullFinished(real distance)

    // What the display's cutout takes at the top of the screen, which the head row is
    // taller by, as the real grid's is.
    property real cutoutHeight: 0
    readonly property real cellWidth: width / 2
    // The picture's inset from its cell's edges, as TabPreview's.
    readonly property real inset: Theme.paddingMedium + Theme.paddingSmall / 2

    SilicaFlickable {
        id: flickable

        readonly property real overscroll: Math.max(0, originY - contentY)
        property real pullDistance: 0

        objectName: "tutorialGridView"
        width: parent.width
        height: parent.height
        y: -overscroll
        contentHeight: height
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

        Grid {
            y: headRow.height
            columns: 2

            Repeater {
                model: 4

                Item {
                    width: tutorialGrid.cellWidth
                    height: tutorialGrid.cellWidth + Theme.itemSizeSmall

                    // The tab in front, washed as the grid washes it.
                    Rectangle {
                        anchors {
                            fill: parent
                            margins: tutorialGrid.inset - Theme.paddingSmall
                        }
                        visible: index === 0
                        color: Theme.rgba(Theme.highlightBackgroundColor,
                                          Theme.highlightBackgroundOpacity)
                    }

                    Rectangle {
                        anchors {
                            fill: parent
                            margins: tutorialGrid.inset
                        }
                        radius: Theme.paddingMedium
                        color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
                    }
                }
            }
        }
    }

    Rectangle {
        id: headRow

        anchors {
            left: parent.left
            right: parent.right
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
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: Theme.itemSizeLarge
        color: Theme.highlightDimmerColor

        GlassTexture {
            anchors.fill: parent
        }
    }
}
