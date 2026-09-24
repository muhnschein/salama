// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Room at the foot of a picture for what is drawn over it there: as a layer's effect,
// the picture runs out towards its bottom edge over the band, and whatever is behind
// it shows through. Eased as piirit's cover eases its faces away under its quick
// actions -- the strength goes as the square of what is left of the band -- since a
// straight ramp reads as a wash laid over the picture. Nothing is laid on top: the
// picture gets out of the way.
//
// One gradient, as the cover's field fades under its heading (CoverTabField.qml),
// stepped in quarters for the curve.
import QtQuick 2.6
import QtGraphicalEffects 1.0

OpacityMask {
    id: fade

    // How far up from the bottom edge the picture starts to go, in pixels.
    property real band: 0
    // Where that is, down the picture; kept short of the foot so the stops below it
    // cannot meet and leave the gradient with a pair out of order.
    readonly property real from: height > 0 ? Math.min(0.96, Math.max(0, 1 - band / height))
                                            : 0

    maskSource: LinearGradient {
        width: fade.width
        height: fade.height
        start: Qt.point(0, 0)
        end: Qt.point(0, fade.height)
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "white"
            }
            GradientStop {
                position: fade.from
                color: "white"
            }
            GradientStop {
                position: fade.from + (1 - fade.from) * 0.25
                color: Qt.rgba(1, 1, 1, 0.5625)
            }
            GradientStop {
                position: fade.from + (1 - fade.from) * 0.5
                color: Qt.rgba(1, 1, 1, 0.25)
            }
            GradientStop {
                position: fade.from + (1 - fade.from) * 0.75
                color: Qt.rgba(1, 1, 1, 0.0625)
            }
            GradientStop {
                position: 1.0
                color: "transparent"
            }
        }
    }
}
