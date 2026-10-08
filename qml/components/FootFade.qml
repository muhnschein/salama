// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Quadratic ease: linear ramp reads as wash. Gradient stepped in quarters.
import QtQuick 2.6
import QtGraphicalEffects 1.0

OpacityMask {
    id: fade

    // px from bottom.
    property real band: 0
    // Capped at 0.96 so later stops can't meet and go out of order.
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
