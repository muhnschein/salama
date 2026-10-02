// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the link sheet lays over the browsing page above itself, laid there rather than
// in the sheet so that the sheet's slide does not carry it
// (docs/DECISIONS/0046-link-menu.md): a dim; the page in front as it was, a still of it,
// while a preview is drawn in its place; and a picture pressed, lifted out of the page and
// grown to the room above the sheet -- edge to edge for a wide one, as tall as there is
// room for a tall one -- which two fingers pinch to look closer.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: overlay

    // Shown while the sheet is.
    property bool shown: false
    // The room above the sheet: from under the cutout to where the sheet comes up to.
    property real roomTop: 0
    property real roomBottom: 0
    // A still of the page in front, an ItemGrabResult, and where that page lies; drawn
    // while stillShown.
    property var still: null
    property rect stillRect
    property bool stillShown: false
    // The address of the picture pressed, or empty.
    property string picture

    objectName: "linkMenuOverlay"
    visible: opacity > 0
    opacity: shown ? 1.0 : 0.0

    Behavior on opacity {
        FadeAnimation {}
    }

    Image {
        objectName: "linkMenuStill"
        x: overlay.stillRect.x
        y: overlay.stillRect.y
        width: overlay.stillRect.width
        height: overlay.stillRect.height
        visible: overlay.stillShown && overlay.still !== null
        source: overlay.still !== null ? overlay.still.url : ""
    }

    Rectangle {
        objectName: "linkMenuDim"
        anchors.fill: parent
        color: Theme.highlightDimmerColor
        opacity: Theme.opacityLow
    }

    Item {
        objectName: "linkMenuPictureArea"
        y: overlay.roomTop
        width: parent.width
        height: Math.max(0, overlay.roomBottom - overlay.roomTop)

        // Centred, so that a pinch grows it from the middle of what was in reach.
        Image {
            id: lifted

            objectName: "linkMenuPicture"
            anchors.centerIn: parent
            width: parent.width
            height: parent.height - 2 * Theme.paddingLarge
            source: overlay.picture
            sourceSize.width: width
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            visible: status === Image.Ready
        }

        PinchArea {
            objectName: "linkMenuPinch"
            anchors.fill: lifted
            enabled: lifted.visible
            pinch.target: lifted
            pinch.minimumScale: 1.0
            pinch.maximumScale: 4.0
            pinch.dragAxis: Pinch.NoDrag
        }
    }

    // A new picture starts at its own size.
    onPictureChanged: lifted.scale = 1.0
}
