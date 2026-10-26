// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Round-capped straight strokes in own box: BadgedIcon's sign, and its cut-out widened.
import QtQuick 2.6

Item {
    id: box

    // [x1, y1, x2, y2] each, fractions of box.
    property var strokes: []
    property real thickness: 1
    property color color

    Repeater {
        model: box.strokes

        Rectangle {
            readonly property real dx: (modelData[2] - modelData[0]) * box.width
            readonly property real dy: (modelData[3] - modelData[1]) * box.height

            objectName: "menuButtonBadgeStroke"
            x: modelData[0] * box.width - box.thickness / 2
            y: modelData[1] * box.height - box.thickness / 2
            width: Math.sqrt(dx * dx + dy * dy) + box.thickness
            height: box.thickness
            radius: box.thickness / 2
            antialiasing: true
            color: box.color
            transform: Rotation {
                origin.x: box.thickness / 2
                origin.y: box.thickness / 2
                angle: Math.atan2(dy, dx) * 180 / Math.PI
            }
        }
    }
}
