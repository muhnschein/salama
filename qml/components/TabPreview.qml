// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One MouseArea under contents splits gestures: tap opens; ~1 s hold picks up to reorder
// or drop on group strip; left drag closes past threshold; vertical drag is grid's.
import QtQuick 2.6
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: preview

    // MouseArea clicked carries mouse event handler can't supply.
    signal tapped()
    signal closeRequested()
    signal muteToggled()
    signal moveRequested(int from, int to)

    property Item dropTarget: null

    // Stay true until next press: released fires before clicked, so it can't suppress tap.
    property bool held: false
    property bool carried: false
    property bool swiping: false
    // Vertical drift bounded by grid's own drag threshold.
    readonly property int holdInterval: 1000
    readonly property real holdTolerance: Theme.iconSizeSmall
    property bool holding: false
    // Mostly sideways past most of grid's drag distance: touch is cell's, never grid scroll.
    property bool sideways: false
    readonly property real closeDistance: width / 3
    readonly property real inset: Theme.paddingMedium + Theme.paddingSmall / 2
    readonly property real frameGap: Theme.paddingSmall / 2
    // Cell outliving its closed row sees undefined role; bool can't be.
    readonly property bool highlighted: dragArea.pressed || model.activeTab === true
    readonly property Item grid: GridView.view

    objectName: "tabPreview"
    width: GridView.view.cellWidth
    height: GridView.view.cellHeight
    z: held ? 1 : 0

    function releaseTap() {
        if (!carried) {
            tapped()
        }
    }

    function pickUp() {
        holdTimer.stop()
        holding = false
        held = true
        carried = true
    }

    function letGo() {
        holdTimer.stop()
        holding = false
    }

    function swipeTo(x) {
        swiping = true
        carried = true
        content.x = Math.min(0, x)
    }

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
        // Steal only once carried, sliding or sideways: flickable refused a touch gives it
        // up for good, so stealing from press kills grid scroll and pull-back. Sideways
        // claimed at 0.75 of Qt's (not Silica's) startDragDistance, which grid measures by.
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
                // Beyond tolerance: hold off; sideways starts slide (left only).
                if (Math.abs(acrossX) <= preview.holdTolerance
                        && Math.abs(acrossY) <= preview.holdTolerance) {
                    return
                }
                preview.letGo()
                if (Math.abs(acrossX) > Math.abs(acrossY)) {
                    preview.swipeTo(acrossX)
                }
            } else if (preview.held) {
                // Move contents, not cell: view owns cell positions.
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
        opacity: 1 - Math.min(1, -x / preview.width)
        scale: preview.held ? 1.05 : 1

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

        Rectangle {
            objectName: "tabPreviewFrame"
            anchors {
                fill: shot
                margins: -(preview.frameGap + border.width)
            }
            radius: shot.radius + preview.frameGap + border.width
            color: "transparent"
            border {
                width: Theme._lineWidth
                color: Theme.highlightBackgroundColor
            }
            visible: preview.highlighted
        }

        Rectangle {
            id: shot

            objectName: "tabPreviewShot"
            anchors {
                fill: parent
                margins: preview.inset
            }
            clip: true
            radius: Theme.paddingMedium
            color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)

            // Clipping is rectangular, so corners cut by mask.
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: Rectangle {
                    width: shot.width
                    height: shot.height
                    radius: shot.radius
                    visible: false
                }
            }

            Item {
                objectName: "tabPreviewPicture"
                anchors.fill: parent
                layer.enabled: muteAction.visible
                layer.effect: FootFade {
                    band: muteAction.height * 2
                }

                // Top-anchored at own aspect: PreserveAspectCrop would centre on page middle.
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
            }

            Label {
                objectName: "tabPreviewPlaceholder"
                anchors.centerIn: parent
                visible: model.thumbnail.length === 0
                text: model.url.length === 0 ? qsTr("Start page") : qsTr("No preview")
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
            }

            // Hand-drawn: icon-m-clear has own translucent disc, lost on pages.
            PreviewButton {
                objectName: "closeTabButton"
                markName: "closeTabMark"
                anchors {
                    right: parent.right
                    top: parent.top
                }
                onClicked: preview.closeRequested()

                Repeater {
                    model: 2

                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width * 2 / 5
                        height: Theme._lineWidth
                        radius: height / 2
                        rotation: index === 0 ? 45 : -45
                        color: Theme.primaryColor
                    }
                }
            }

            // Roles undefined for cell outliving its row.
            PreviewMuteAction {
                id: muteAction

                objectName: "previewMuteAction"
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    bottom: parent.bottom
                }
                mediaState: model.mediaState === undefined ? TabModel.NoMedia : model.mediaState
                muted: model.muted === true
                onToggled: preview.muteToggled()
            }
        }
    }
}
