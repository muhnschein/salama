// Stub: Silica's ring that fills as something goes along, value from 0 to 1. Nothing
// is drawn; the values are there for the tests to read.
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
