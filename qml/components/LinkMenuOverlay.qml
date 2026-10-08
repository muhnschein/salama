// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Outside sheet so sheet's slide doesn't move it.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: overlay

    property bool shown: false
    property real roomTop: 0
    property real roomBottom: 0
    // ItemGrabResult of front page plus its position; drawn while stillShown.
    property var still: null
    property rect stillRect
    property bool stillShown: false
    property string picture
    property real pictureZ
    // Drawn picture's top, in parent's coordinates.
    readonly property bool pictureShown: lifted.visible
    readonly property real pictureTop: pictureArea.y + pinchArea.y

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

    // Over sheet and its shade, so drawn undimmed. Clipped so pinch zoom stays out of sheet.
    Item {
        id: pictureArea

        objectName: "linkMenuPictureArea"
        parent: overlay.parent
        z: overlay.pictureZ
        visible: overlay.visible
        opacity: overlay.opacity
        clip: true
        y: overlay.roomTop
        width: parent.width
        height: Math.max(0, overlay.roomBottom - overlay.roomTop)

        // Centred so pinch grows from middle.
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

        // Drawn picture only, not its letterbox.
        PinchArea {
            id: pinchArea

            objectName: "linkMenuPinch"
            anchors.centerIn: lifted
            width: lifted.paintedWidth * lifted.scale
            height: lifted.paintedHeight * lifted.scale
            enabled: lifted.visible
            pinch.target: lifted
            pinch.minimumScale: 1.0
            pinch.maximumScale: 4.0
            pinch.dragAxis: Pinch.NoDrag

            MouseArea {
                objectName: "linkMenuPictureTap"
                anchors.fill: parent
                onDoubleClicked: {
                    zoom.to = lifted.scale > 1.0 ? 1.0 : 2.5
                    zoom.restart()
                }
            }
        }

        NumberAnimation {
            id: zoom

            target: lifted
            property: "scale"
            duration: 200
            easing.type: Easing.InOutQuad
        }
    }

    onPictureChanged: {
        zoom.stop()
        lifted.scale = 1.0
    }
}
