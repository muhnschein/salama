// Stub: Silica progress ring, value 0..1. Draws nothing.
import QtQuick 2.6

Item {
    property real value
    property real progressValue: Math.max(0.0, Math.min(value, 1.0))
    property color progressColor
    property color backgroundColor
    property real borderWidth: 6
    property bool inAlternateCycle: false

    width: 80
    height: 80
}
