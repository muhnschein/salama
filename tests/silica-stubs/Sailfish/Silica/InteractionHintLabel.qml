// Stub: parent-wide band, clear at one edge, dims toward other where highlight-colour text
// sits: bottom, or top when inverted.
import QtQuick 2.6

Rectangle {
    property string text
    property bool invert: false
    property color textColor
    property color backgroundColor

    width: parent ? parent.width : 0
}
