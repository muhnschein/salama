// Stub: the module is not installed on the host, and these tests draw nothing. It
// exists so the import resolves; what the effect renders is a device question.
import QtQuick 2.6

Item {
    property variant source
    property real horizontalOffset: 0
    property real verticalOffset: 0
    property real horizontalRadius: width
    property real verticalRadius: height
    property real angle: 0
    property Gradient gradient
    property bool cached: false
}
