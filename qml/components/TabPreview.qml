// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One cell of the tab grid: the captured page preview with a close button in its
// top-right corner, and the favicon and title underneath.
//
// Three gestures share the cell, and one MouseArea under the contents tells them
// apart the way the navigation bar's does. A tap opens the tab. A finger held for a
// second -- still, or near enough: a thumb held down drifts -- picks the cell up to
// be carried to another place in the grid, or onto a group in the strip at its
// foot, which the tab moves into when it is dropped there. A drag to the left slides
// the cell out and closes the tab when it has gone far enough; released short of
// that it slides back. A drag up or down is none of these: it is the grid's, to
// scroll or to hand the page back. The button is drawn above the handler and keeps
// its own taps (docs/DECISIONS/0010-tab-grid-deck.md).
//
// It is a plain Item rather than a Silica BackgroundItem. That one draws its press
// and its highlight as a wash across the whole cell, edge to edge; the wash here is as
// square, but stops short of the edges, round the picture and the title.
import QtQuick 2.6
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0

Item {
    id: preview

    // Its own tap signal: a Silica clicked() comes from MouseArea and carries a mouse
    // event, which this handler has not got to give it.
    signal tapped()
    signal closeRequested()
    // The cell has been carried over another one and the two should trade places.
    signal moveRequested(int from, int to)

    // Where a carried cell can go besides among its neighbours: the strip of groups.
    property Item dropTarget: null

    // True while this cell is being carried, and true from the moment a press turns
    // into a carry or a swipe until the next press. MouseArea raises released before
    // clicked, so the first cannot be what keeps the second from opening the tab.
    property bool held: false
    property bool carried: false
    // True while the cell is being slid out to the left.
    property bool swiping: false
    // How long a finger holds before the cell comes up, and how far it may drift
    // sideways meanwhile and still count as holding. Up and down it may drift as far
    // as the grid lets a finger move before taking it as a drag.
    readonly property int holdInterval: 1000
    readonly property real holdTolerance: Theme.iconSizeSmall
    // True from the press until the finger has moved too far to be holding.
    property bool holding: false
    // True once the finger has moved more sideways than up or down, by most of the
    // distance at which the grid would take it: from then on the touch is the cell's,
    // a hold or a slide, and never the grid's scroll.
    property bool sideways: false
    // How far the cell must be slid before letting go closes the tab.
    readonly property real closeDistance: width / 3
    // The picture and title's inset from the cell's edges, half the gap between cells.
    readonly property real inset: Theme.paddingMedium + Theme.paddingSmall / 2
    // What the wash and the border below mark: this cell is the active tab, or has a
    // finger. A cell whose tab has just been closed outlives its row for a moment, and
    // its role is then undefined, which a bool cannot be.
    readonly property bool highlighted: dragArea.pressed || model.activeTab === true
    readonly property Item grid: GridView.view

    objectName: "tabPreview"
    width: GridView.view.cellWidth
    height: GridView.view.cellHeight
    // A carried cell passes over its neighbours, not under them.
    z: held ? 1 : 0

    // A cell that has been carried or slid must not also open on release.
    function releaseTap() {
        if (!carried) {
            tapped()
        }
    }

    // The hold has run its course: the cell is the finger's to carry.
    function pickUp() {
        holdTimer.stop()
        holding = false
        held = true
        carried = true
    }

    // The finger has moved too far to be holding: the hold is off.
    function letGo() {
        holdTimer.stop()
        holding = false
    }

    // The cell slid this far to the left, by a finger or by a test.
    function swipeTo(x) {
        swiping = true
        carried = true
        content.x = Math.min(0, x)
    }

    // The finger lifted off a slide: closed if it went far enough, back if not.
    function releaseSwipe() {
        swiping = false
        if (-content.x >= closeDistance) {
            closeRequested()
        } else {
            content.x = 0
        }
    }

    function drop() {
        letGo()
        held = false
        sideways = false
        content.x = 0
        content.y = 0
    }

    Timer {
        id: holdTimer

        objectName: "holdTimer"
        interval: preview.holdInterval
        onTriggered: preview.pickUp()
    }

    MouseArea {
        id: dragArea

        property real grabX: 0
        property real grabY: 0

        objectName: "tabPreviewGesture"
        anchors.fill: parent
        // Once the cell is carried or sliding, or the finger has gone sideways, the
        // grid may not take the drag back. Not before: a flickable that is refused a
        // touch once gives up on it for good, so a cell that kept every press from the
        // start left the grid nothing to scroll and nothing to pull the page back with,
        // for any drag begun on a cell. A drag up or down therefore cancels the hold,
        // as it does on any Silica list. Sideways is claimed short of the grid's own
        // drag distance, Qt's rather than Silica's, since that is the one the grid
        // measures by: a slide that slants would otherwise be the grid's before it
        // had gone far enough across to be a slide.
        preventStealing: preview.held || preview.swiping || preview.sideways

        onPressed: {
            grabX = mouse.x
            grabY = mouse.y
            preview.carried = false
            preview.swiping = false
            preview.sideways = false
            preview.holding = true
            holdTimer.restart()
        }
        onPositionChanged: {
            var acrossX = mouse.x - grabX
            var acrossY = mouse.y - grabY
            if (!preview.held && !preview.swiping) {
                if (Math.abs(acrossX) > Math.abs(acrossY)
                        && Math.abs(acrossX) > Qt.styleHints.startDragDistance * 0.75) {
                    preview.sideways = true
                }
                // Within the tolerance the finger is still holding. Beyond it the
                // hold is off, and a sideways move is the start of a slide. Leftwards
                // only -- the grid has nothing to the right -- while an up-and-down
                // move is the grid's own, and it has usually taken it before here.
                if (Math.abs(acrossX) <= preview.holdTolerance
                        && Math.abs(acrossY) <= preview.holdTolerance) {
                    return
                }
                preview.letGo()
                if (Math.abs(acrossX) > Math.abs(acrossY)) {
                    preview.swipeTo(acrossX)
                }
            } else if (preview.held) {
                // The contents move, not the cell: the view owns where cells are, and
                // after a trade the cell underneath has already moved to meet them.
                content.x = acrossX
                content.y = acrossY
                if (preview.dropTarget && preview.dropTarget.carryOver(preview, mouse.x, mouse.y)) {
                    return
                }
                var target = preview.grid.indexAt(preview.x + mouse.x, preview.y + mouse.y)
                if (target >= 0 && target !== index) {
                    preview.moveRequested(index, target)
                }
            } else {
                content.x = Math.min(0, acrossX)
            }
        }
        onReleased: {
            if (preview.swiping) {
                preview.releaseSwipe()
            } else if (preview.held && preview.dropTarget) {
                preview.dropTarget.dropTab(model.tabId)
            }
            preview.drop()
        }
        onCanceled: {
            preview.swiping = false
            preview.drop()
        }
        onClicked: preview.releaseTap()
    }

    Item {
        id: content

        width: parent.width
        height: parent.height
        // A cell slid away fades as it goes, so the finger sees what letting go
        // will do.
        opacity: 1 - Math.min(1, -x / preview.width)
        // A carried cell comes up a little, so the hand knows it has it.
        scale: preview.held ? 1.05 : 1

        // Back into place when released short of closing; not while a finger has
        // it, and not while it is being slid.
        Behavior on x {
            enabled: !dragArea.pressed && !preview.swiping

            NumberAnimation {
                duration: 150
                easing.type: Easing.OutQuad
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: 100
            }
        }

        // What marks the active cell, and the one under a finger: the wash Silica's
        // BackgroundItem would have drawn across the cell, square as that one is, round
        // the picture and the title. The thin border below says the same thing quietly;
        // on device it turned out to say nothing at all on its own.
        Rectangle {
            objectName: "tabPreviewHighlight"
            anchors {
                fill: parent
                margins: preview.inset - Theme.paddingSmall
                // Further down than the rest: the title sat close to the edge of it.
                bottomMargin: preview.inset - Theme.paddingSmall * 1.5
            }
            color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
            visible: preview.highlighted
        }

        Rectangle {
            id: shot

            objectName: "tabPreviewShot"
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                margins: preview.inset
            }
            height: parent.height - caption.height - preview.inset * 2 - Theme.paddingMedium
            clip: true
            radius: Theme.paddingMedium
            color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
            border.width: preview.highlighted ? Theme.paddingSmall / 2 : 0
            border.color: Theme.highlightColor

            // Rounded at the corners, picture and all. Clipping is rectangular
            // whatever the shape of the item doing it, so the corners are cut by a
            // mask instead; sailfish-browser rounds its own tab previews the same way
            // (apps/browser/qml/pages/components/TabItem.qml).
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: Rectangle {
                    width: shot.width
                    height: shot.height
                    radius: shot.radius
                    visible: false
                }
            }

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

            // Shown until the tab has been displayed at least once.
            Label {
                objectName: "tabPreviewPlaceholder"
                anchors.centerIn: parent
                visible: model.thumbnail.length === 0
                text: qsTr("No preview")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }

            // The close button: a disc of the highlight colour with a cross cut through
            // it, faint enough not to be the first thing seen on each cell. Drawn here,
            // not the theme's icon-m-clear: that icon carries a disc of its own at its
            // own transparency, so the glyph alone was lost on most pages and a disc
            // behind it was a disc inside a disc.
            Item {
                id: closeButton

                // Its own tap signal, as the cell has: the handler's carries a mouse
                // event, which a test cannot give it.
                signal clicked()

                objectName: "closeTabButton"
                anchors {
                    right: parent.right
                    top: parent.top
                }
                // The touch target is the whole corner; the mark is what shows.
                width: Theme.iconSizeMedium + Theme.paddingSmall
                height: width
                onClicked: preview.closeRequested()

                MouseArea {
                    id: closeTap

                    anchors.fill: parent
                    onClicked: closeButton.clicked()
                }

                Rectangle {
                    id: closeMark

                    objectName: "closeTabMark"
                    anchors.centerIn: parent
                    width: Theme.iconSizeSmall + Theme.paddingMedium
                    height: width
                    radius: width / 2
                    color: closeTap.pressed ? Theme.highlightColor
                                            : Theme.highlightBackgroundColor
                    opacity: closeTap.pressed ? 1.0 : Theme.opacityHigh

                    Repeater {
                        model: 2

                        Rectangle {
                            anchors.centerIn: parent
                            width: closeMark.width / 2
                            height: Theme.paddingSmall / 2
                            radius: height / 2
                            rotation: index === 0 ? 45 : -45
                            color: Theme.primaryColor
                        }
                    }
                }
            }
        }

        Row {
            id: caption

            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                margins: preview.inset
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
